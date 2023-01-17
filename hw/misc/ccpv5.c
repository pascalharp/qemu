/*
 * AMD PSP emulation
 *
 * Copyright (C) 2020 Robert Buhren <robert@robertbuhren.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <byteswap.h>
#include <zlib.h>
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "sysemu/sysemu.h"
#include "exec/address-spaces.h"
#include "hw/sysbus.h"
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "qemu/log-for-trace.h"
#include "qemu/log.h"
#include "hw/misc/ccpv5.h"
#include "hw/misc/ccpv5-linux.h"
#include "hw/misc/ccpv5-nettle.h"
#include "trace-hw_misc.h"
#include "trace.h"
#include "hw/arm/psp.h"
#include "crypto/hash.h"
/* TODO: Restrict access to specific MemoryRegions only.
 *       Verify correct use of cpu_physical_memory_map/unmap */

/* TODO Document*/
static void ccp_process_q(CcpV5State *s, uint32_t id);

//static void ccp_timer_cb(void *opaque)
//{
//    CcpV5State *s = CCP_V5(opaque);
//
//    if (s->dma_timer_qid != -1) {
//        ccp_process_q(s, s->dma_timer_qid);
//        s->dma_timer_qid = -1;
//    } else {
//        qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Timer fired, but no queue ID set\n");
//
//    }
//}

static void ccp_reverse_buf(uint8_t *buf, size_t len) {
    uint8_t tmp;
    uint8_t* buf_top = buf + len - 1;

    while (buf < buf_top) {
        tmp = *buf;
        *buf = *buf_top;
        *buf_top = tmp;
        buf++;
        buf_top--;
    }
}

/* memcpy with ccp bitwise and byteswap ops */

static void ccp_memcpy(void *dst, void *src, size_t len, ccp_pt_bitwise bwise,
                       ccp_pt_byteswap bswap) {

    if (bwise != CCP_PASSTHRU_BITWISE_NOOP) {
        qemu_log_mask(LOG_UNIMP, "CCP: Error unimplemeted bitwise op: %d\n",
                      bwise);
        return;
    }

    if (bwise == CCP_PASSTHRU_BITWISE_NOOP &&
        bswap == CCP_PASSTHRU_BYTESWAP_NOOP) {
        /* Normal memcpy */
        memcpy(dst, src, len);
        return;
    }

    if (bswap == CCP_PASSTHRU_BYTESWAP_256BIT) {
        /* For now, we only support swap-size aligned ops */
        if(len % 0x20) {
            qemu_log_mask(LOG_UNIMP, "CCP: Error, unaligned bswap op is not" \
                                     " supported yet\n");
            return;
        }
        /* TODO clean up */
        memcpy(dst, src, len);
        ccp_reverse_buf(dst, len);

    }


}

static uint32_t ccp_queue_read(CcpV5State *s, hwaddr offset, uint32_t id) {
    uint32_t ret;
    CcpV5QState *qs;

    qs = &s->q_states[id];
    switch(offset) {
        case CCP_Q_CTRL_OFFSET:
            if(qs->ccp_q_control & CCP_Q_RUN) {
                ccp_process_q(s,id);
            }
            ret = qs->ccp_q_control;
            trace_ccp_queue_ctrl_read(id, ret);
            break;
        case CCP_Q_TAIL_LO_OFFSET:
            ret = qs->ccp_q_tail;
            trace_ccp_queue_tail_lo_read(id, ret);
            break;
        case CCP_Q_HEAD_LO_OFFSET:
            ret = qs->ccp_q_head;
            trace_ccp_queue_head_lo_read(id, ret);
            break;
        case CCP_Q_STATUS_OFFSET:
            ret = qs->ccp_q_status;
            trace_ccp_queue_status_read(id, ret);
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "CCP: CCP queue read at unknown " \
                          "offset: 0x%" HWADDR_PRIx "\n", offset);
            ret = 0;
    }
    return ret;
} 

static void ccp_in_guest_pt(CcpV5State *s, uint32_t id, hwaddr dst, hwaddr src,
                            uint32_t len, ccp_memtype dst_type,
                            ccp_memtype src_type, ccp_pt_bitwise bwise,
                            ccp_pt_byteswap bswap) {
    /* TODO: Ensure that we don't access un-accessible memory regions *
     *       Test whether dst - dst + len is within SRAM.
     *       Verify that we don't overflow any host memory.
     *       Cleanup mappings on early return.
     */
    void* hdst = NULL;
    void* hsrc = NULL;
    hwaddr plen = len;

    bool dclean = false;
    bool sclean = false;

    if (bwise != CCP_PASSTHRU_BITWISE_NOOP ||
        (bswap != CCP_PASSTHRU_BYTESWAP_NOOP &&
        bswap != CCP_PASSTHRU_BYTESWAP_256BIT)) {

        qemu_log_mask(LOG_UNIMP, "CCP: Unimplemented passthrough bit ops: " \
                      "bitwise 0x%x byteswap 0x%x\n", bwise, bswap);
        return;

    }

    if (dst_type == CCP_MEMTYPE_SYSTEM || src_type == CCP_MEMTYPE_SYSTEM) {
        qemu_log_mask(LOG_UNIMP, "CCP: Unimplemented memtype CCP_MEMTYPE_SYSTEM\n");
        return;
    }

    /* Create dst pointer */
    if (dst_type == CCP_MEMTYPE_SB) {
        if((dst + len) > sizeof(s->lsb.u.lsb)) {
            /* The access exceeds the LSB size: Cap the len at LSB max. TODO */
            qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Access exceeds LSB size \n");
            return;
        }
        hdst = s->lsb.u.lsb + dst;
    } else if (dst_type == CCP_MEMTYPE_LOCAL) {
        /* TODO: Check whether "dst" is accessible for the CCP */
        /* sets "plen" to the length of memory that was acutally mapped */
        hdst = cpu_physical_memory_map(dst, &plen, true);
        if (hdst == NULL) {
            qemu_log_mask(LOG_GUEST_ERROR, "CCP: Couldn't map guest memory during" \
                                           " passthrough operation\n");
            return;
        }
        dclean = true;
    }

    /* Create src pointer */
    if (src_type == CCP_MEMTYPE_SB) {
        if((src + len) > sizeof(s->lsb.u.lsb)) {
            /* The access exceeds the LSB size: Cap the len at LSB max. TODO */
            qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Access exceeds LSB size \n");
            return;
        }
        hsrc = s->lsb.u.lsb + src;
    }
    else if (src_type == CCP_MEMTYPE_LOCAL) {
        /* TODO: Check whether "src" is accessible for the CCP */
        /* sets "plen" to the length of memory that was acutally mapped */
        hsrc = cpu_physical_memory_map(src, &plen, false);
        if (hsrc == NULL) {
            qemu_log_mask(LOG_GUEST_ERROR, "CCP: Couldn't map guest memory during" \
                                           " passthrough operation\n");
            return;
        }
        sclean = true;
    }


    /* "plen" might have been shortened to the range of memory actually available */
    if (plen < len) {
        qemu_log_mask(LOG_GUEST_ERROR, "CCP: Only copying 0x%x bytes!\n", (uint32_t) plen);
    }
    ccp_memcpy(hdst, hsrc, plen, bwise, bswap);

    if (dclean)
        cpu_physical_memory_unmap(hdst, len, true, 0);
    if (sclean)
        cpu_physical_memory_unmap(hsrc, len, false, 0);
}


static void ccp_passthrough(CcpV5State *s, uint32_t id, ccp5_desc *desc) {
    /* TODO remove this function? */
    ccp_function func;
    ccp_memtype src_type;
    ccp_memtype dst_type;
    hwaddr src;
    hwaddr dst;
    uint32_t cbytes;
    ccp_pt_bitwise bwise;
    ccp_pt_byteswap bswap;

    func.raw = CCP5_CMD_FUNCTION(desc);
    src_type = CCP5_CMD_SRC_MEM(desc);
    dst_type = CCP5_CMD_DST_MEM(desc);
    cbytes   = CCP5_CMD_LEN(desc);
    /* TODO: Does the shift maybe cause issues? */
    src = CCP5_CMD_SRC_LO(desc) | ((hwaddr)(CCP5_CMD_SRC_HI(desc)) << 32);
    dst = CCP5_CMD_DST_LO(desc) | ((hwaddr)(CCP5_CMD_DST_HI(desc)) << 32);
    bwise = func.pt.bitwise;
    bswap = func.pt.byteswap;

    trace_ccp_passthrough(src, dst, cbytes);
    ccp_in_guest_pt(s, id, dst, src, cbytes, dst_type, src_type, bwise, bswap);
}

static void ccp_perform_sha_256(CcpV5State *s, hwaddr src, uint32_t len, bool eom, bool init, uint8_t* lsb_ctx) {
    /* TODO: use nettle. See: https://www.gnutls.org/manual/html_node/Using-GnuTLS-as-a-cryptographic-library.html#Using-GnuTLS-as-a-cryptographic-library */
    /* Use "init" to determine whether we need to initialize a new sha context */
    void* hsrc;
    hwaddr plen;
    plen = len;

    if (s->sha_ctx.raw == NULL) {
        /* Init new SHA256 context */
        ccp_init_sha256_ctx(&s->sha_ctx);
    }
    if(s->sha_ctx.raw != NULL) {
        hsrc = cpu_physical_memory_map(src, &plen, false);
        if (plen != len) {
            qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Couldn't map guest memory\n");
            return;
        }
        if (hsrc != NULL) {
            ccp_update_sha256(&s->sha_ctx, len, hsrc);
        } else {
            qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Couldn't map guest memory\n");
            return;
        }
        if (eom) {
            ccp_digest_sha256(&s->sha_ctx, lsb_ctx);
            /* The CCP seems to store the digest in reversed order -> Do as the
             * CCP would do, even if it means we reverse it again when the 
             * digest is copied from the lsb to the PSP.
             */
            ccp_reverse_buf(lsb_ctx, 32); // 32 -> SHA256 digest size 
            cpu_physical_memory_unmap(hsrc, plen, false, 0);
        }

    } else {
        qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: SHA context not initialized\n");
        return;
    }
}

static void ccp_perform_sha_384(CcpV5State *s, hwaddr src, uint32_t len, bool eom, bool init, uint8_t* lsb_ctx) {
    /* TODO: use nettle. See: https://www.gnutls.org/manual/html_node/Using-GnuTLS-as-a-cryptographic-library.html#Using-GnuTLS-as-a-cryptographic-library */
    /* Use "init" to determine whether we need to initialize a new sha context */
    void* hsrc;
    hwaddr plen;
    plen = len;

    if (s->sha_ctx.raw == NULL) {
        /* Init new SHA384 context */
        ccp_init_sha384_ctx(&s->sha_ctx);
    }
    if(s->sha_ctx.raw != NULL) {
        hsrc = cpu_physical_memory_map(src, &plen, false);
        if (plen != len) {
            qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Couldn't map guest memory\n");
            return;
        }
        if (hsrc != NULL) {
            ccp_update_sha384(&s->sha_ctx, len, hsrc);
        } else {
            qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Couldn't map guest memory\n");
            return;
        }
        if (eom) {
            ccp_digest_sha384(&s->sha_ctx, lsb_ctx);
            /* The CCP seems to store the digest in reversed order -> Do as the
             * CCP would do, even if it means we reverse it again when the 
             * digest is copied from the lsb to the PSP.
             */
            ccp_reverse_buf(lsb_ctx, 48); // 48 -> SHA384 digest size 
            cpu_physical_memory_unmap(hsrc, plen, false, 0);
        }

    } else {
        qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: SHA context not initialized\n");
        return;
    }
}

static bool ccp_zlib(CcpV5State *s, uint32_t id, ccp5_desc *desc) {
    bool init;
    bool eom;
    ccp_memtype src_type;
    ccp_memtype dst_type;
    hwaddr src;
    hwaddr dst;
    uint32_t len;
    void* hsrc;
    void* hdst;
    hwaddr plen_in;
    hwaddr plen_out;
    bool err;

    src_type = CCP5_CMD_SRC_MEM(desc);
    dst_type = CCP5_CMD_DST_MEM(desc);
    len = CCP5_CMD_LEN(desc);
    plen_in = len;
    /* TODO: Does the shift maybe cause issues? */
    src = CCP5_CMD_SRC_LO(desc) | ((hwaddr)(CCP5_CMD_SRC_HI(desc)) << 32);
    dst = CCP5_CMD_DST_LO(desc) | ((hwaddr)(CCP5_CMD_DST_HI(desc)) << 32);
    init = CCP5_CMD_INIT(desc);
    eom = CCP5_CMD_EOM(desc);


    trace_ccp_zlib(src, dst, len, src_type, dst_type, init, eom);

    if (!init || !eom) {
        qemu_log_mask(LOG_UNIMP, "CCP Error: Only EOM=1 and INIT=1 Zlib ops are currently supported\n");
        return true;

    }

    if (init) {
      if (ccp_zlib_init(&s->zlib_state)) {
        qemu_log_mask(LOG_UNIMP, "CCP Error: Couldn't initialize zlib state\n");
        return true;
      }
    }

    /* Mapping input buffer */
    hsrc = cpu_physical_memory_map(src, &plen_in, false);
    if (plen_in != len) {
        qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Zlib couldn't map guest input memory\n");
        cpu_physical_memory_unmap(hsrc, plen_in, true, plen_in);
        return true;
    }
    /* Mapping output buffer */
    /* TODO: Try to decompress page wise until we can't map the hosts memory anymore */
    plen_out = CCP_ZLIB_CHUNK_SIZE;
    hdst = cpu_physical_memory_map(dst, &plen_out, false);
    if (plen_out != CCP_ZLIB_CHUNK_SIZE) {
      /* qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Zlib couldn't map guest output memory\n"); */
      err = true;
        /* return; */
    }

    ccp_zlib_inflate(&s->zlib_state, plen_in, plen_out, hdst, hsrc);

    if(eom) {
      ccp_zlib_end(&s->zlib_state);
    }

    return err;

}

static void ccp_sha(CcpV5State *s, uint32_t id, ccp5_desc *desc) {
    ccp_function func;
    bool init;
    bool eom;
    hwaddr src;
    uint32_t len;
    uint64_t sha_len;
    uint32_t ctx_id;
    uint8_t* ctx;

    func.raw = CCP5_CMD_FUNCTION(desc);
    init = CCP5_CMD_INIT(desc);
    eom = CCP5_CMD_EOM(desc);
    len = CCP5_CMD_LEN(desc);
    src = CCP5_CMD_SRC_LO(desc) | ((hwaddr)(CCP5_CMD_SRC_HI(desc)) << 32);
    sha_len = CCP5_CMD_SHA_LO(desc) | ((uint64_t)(CCP5_CMD_SHA_HI(desc)) << 32);
    ctx_id = CCP5_CMD_LSB_ID(desc);

    ctx = s->lsb.u.slots[ctx_id].data;

    switch (func.sha.type) {
        case CCP_SHA_TYPE_256:
            trace_ccp_sha(src, len, sha_len, init, eom, ctx_id);
            ccp_perform_sha_256(s, src, len, eom, init, ctx);

            break;
        case CCP_SHA_TYPE_384:
            trace_ccp_sha(src, len, sha_len, init, eom, ctx_id);
            ccp_perform_sha_384(s, src, len, eom, init, ctx);

            break;
        default:
            qemu_log_mask(LOG_UNIMP, "CCP Error. Unimplemented SHA type %d\n", 
                          func.sha.type);
            break;

    }

}

/* TODO: Check all map/unmap for whether the mapping actually succeeded */
static void ccp_copy_to_host(CcpV5State *s, void* hdst, hwaddr src, ccp_memtype stype, size_t len) {
    void *hsrc = NULL;
    hwaddr plen = len;

    if (stype == CCP_MEMTYPE_LOCAL) {
        hsrc = cpu_physical_memory_map(src, &plen, false);
        memcpy(hdst, hsrc, len);
        cpu_physical_memory_unmap(hsrc, len, false, plen);
    } else if (stype == CCP_MEMTYPE_SB) {
        hsrc = s->lsb.u.lsb + src;
        memcpy(hdst, hsrc, len);

    }


}

static void ccp_copy_to_guest(CcpV5State *s, hwaddr dst, void* hsrc, ccp_memtype dtype, size_t len) {
    void *hdst = NULL;
    hwaddr plen = len;

    if(dtype == CCP_MEMTYPE_LOCAL) {
        /* TODO: Check for error */
        hdst = cpu_physical_memory_map(dst, &plen, true);
        memcpy(hdst, hsrc, plen);
        cpu_physical_memory_unmap(hdst, plen, true, plen);
    } else if (dtype == CCP_MEMTYPE_SB) {
        hdst = s->lsb.u.lsb + dst;
        memcpy(hdst, hsrc, len);
    }

}

/* TODO use ccp_copy_to_host instead */
/* static void ccp_get_key(CcpV5State* s,ccp5_desc *desc, void *buffer, size_t key_len) { */
/*     hwaddr key_addr; */
/*     hwaddr klen = key_len; */
/*     void* src = NULL; */

/*     key_addr = CCP5_CMD_KEY_LO(desc) | ((hwaddr)(CCP5_CMD_KEY_HI(desc)) << 32); */
/*     if (CCP5_CMD_KEY_MEM(desc) == CCP_MEMTYPE_LOCAL) { */
/*         src = cpu_physical_memory_map(key_addr, &klen, false); */
/*         if (klen != key_len) { */
/*             qemu_log_mask(LOG_GUEST_ERROR, "CCP Error. Couldn't map guest memory\n"); */
/*         } */
/*         memcpy(buffer, src, key_len); */
/*         cpu_physical_memory_unmap(src, klen, false, klen); */
/*     } else if (CCP5_CMD_KEY_MEM(desc) == CCP_MEMTYPE_SB) { */
/*         /1* TODO Ensure correct length *1/ */
/*         src = s->lsb.u.lsb + key_addr; */
/*         memcpy(buffer, src, key_len); */
/*     } */


/* } */

static void ccp_rsa(CcpV5State *s, uint32_t id, ccp5_desc *desc) {
    uint16_t rsa_size;
    uint16_t rsa_mode;
    ccp_function func;
    bool init;
    bool eom;
    uint32_t len;
    ccp_memtype rsa_key_mtype;
    ccp_memtype rsa_src_mtype;
    ccp_memtype rsa_dst_mtype;
    hwaddr rsa_key_addr;
    hwaddr rsa_src;
    hwaddr rsa_dst;
    CcpV5RsaPubKey rsa_pubkey;

    uint8_t rsa_exp[512]; /* Only 2048 & 4096 for now */
    uint8_t rsa_mod[512]; /* Only 2048 & 4096 for now */
    uint8_t rsa_msg[512];           /* Assuming it is a signature with length equal to the modulus */
    uint8_t rsa_result[512];



    func.raw = CCP5_CMD_FUNCTION(desc);
    init = CCP5_CMD_INIT(desc);
    eom = CCP5_CMD_EOM(desc);
    len = CCP5_CMD_LEN(desc);
    rsa_size = func.rsa.size;
    rsa_mode = func.rsa.mode;
    rsa_key_mtype = CCP5_CMD_KEY_MEM(desc);
    rsa_src_mtype = CCP5_CMD_SRC_MEM(desc);
    rsa_dst_mtype = CCP5_CMD_SRC_MEM(desc);

    rsa_key_addr = CCP5_CMD_KEY_LO(desc) | ((hwaddr)(CCP5_CMD_KEY_HI(desc)) << 32);
    rsa_src = CCP5_CMD_SRC_LO(desc) | ((hwaddr)(CCP5_CMD_SRC_HI(desc)) << 32);
    rsa_dst = CCP5_CMD_DST_LO(desc) | ((hwaddr)(CCP5_CMD_DST_HI(desc)) << 32);

    trace_ccp_rsa(desc->src_lo, len, rsa_dst, rsa_dst_mtype, rsa_mode, rsa_size, eom, init);

    if (rsa_mode == 0 && 
            ( (rsa_size == 256 && len == 512) || (rsa_size == 512 && len == 1024)) ) {

        /* TODO consistent naming */
        rsa_pubkey.rsa_len = rsa_size;

        /* TODO: Use guest memory ?*/

        /* Copy RSA exponent */
        ccp_copy_to_host(s, rsa_exp, rsa_key_addr, rsa_key_mtype, rsa_size);

        /* Copy RSA modulus
         * "rsa_src" contains the modulus, follwed by the message
         */
        ccp_copy_to_host(s, rsa_mod, rsa_src, rsa_src_mtype, rsa_size);

        /* Copy RSA message (signature) */
        ccp_copy_to_host(s, rsa_msg, rsa_src + rsa_size , rsa_src_mtype, rsa_size);

        ccp_rsa_init_key(&rsa_pubkey, rsa_mod, rsa_exp);


        ccp_rsa_encrypt(&rsa_pubkey, rsa_msg, rsa_result);

        ccp_rsa_clear_key(&rsa_pubkey);

        ccp_copy_to_guest(s, rsa_dst, rsa_result, rsa_dst_mtype, rsa_size);

    }

}

static void ccp_execute(CcpV5State *s, uint32_t id, ccp5_desc *desc) {

    ccp_engine engine = CCP5_CMD_ENGINE(desc);


    switch(engine) {
        case CCP_ENGINE_AES:
            qemu_log_mask(LOG_UNIMP, "CCP: Unimplemented engine (AES)\n");
            break;
        case CCP_ENGINE_XTS_AES_128:
            qemu_log_mask(LOG_UNIMP, "CCP: Unimplemented engine (AES-XTS-128)\n");
            break;
        case CCP_ENGINE_DES3:
            qemu_log_mask(LOG_UNIMP, "CCP: Unimplemented engine (DES3)\n");
            break;
        case CCP_ENGINE_SHA:
            ccp_sha(s, id, desc);
            break;
        case CCP_ENGINE_RSA:
            ccp_rsa(s, id, desc);
            break;
        case CCP_ENGINE_PASSTHRU:
            ccp_passthrough(s, id, desc);
            break;
        case CCP_ENGINE_ZLIB_DECOMPRESS:
            ccp_zlib(s, id, desc);
            qemu_log_mask(LOG_UNIMP, "CCP: Unimplemented engine (ZLIB)\n");
            break;
        case CCP_ENGINE_ECC:
            qemu_log_mask(LOG_UNIMP, "CCP: Unimplemented engine (ECC)\n");
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "CCP: Unknown engine! val 0x%x\n", 
                          CCP5_CMD_ENGINE(desc));
            break;
    }

}

static void ccp_process_q(CcpV5State *s, uint32_t id) {

    CcpV5QState *qs;
    ccp5_desc *desc;
    hwaddr req_len;
    uint32_t tail;
    uint32_t head;
    /* TODO: This operation needs to be delayed! */

    qs = &s->q_states[id];
    trace_ccp_process_queue(qs->ccp_q_id, qs->ccp_q_tail, qs->ccp_q_head);

    /* Clear HALT bit */
    /* Clear RUN bit. TODO: The secure OS never re-sets the RUN bit. We need
     * to figure out how/when we really clear this bit */
    qs->ccp_q_control &= ~((CCP_Q_CTRL_HALT | CCP_Q_RUN));

    /* TODO fix tail/head data types? */
    tail = qs->ccp_q_tail;
    head = qs->ccp_q_head;

    req_len = sizeof(ccp5_desc);

    while (tail < head) {
        desc = cpu_physical_memory_map(tail, &req_len, false);
        ccp_execute(s, id, desc);
        /* TODO: What is "access_len" ? */
        cpu_physical_memory_unmap(desc, req_len, false, sizeof(ccp5_desc));
        tail += sizeof(ccp5_desc);

    }

    /* TODO: Return proper CCP error codes */
    qs->ccp_q_tail = qs->ccp_q_head;
    qs->ccp_q_status = CCP_Q_STATUS_SUCCESS;
    qs->ccp_q_control |= CCP_Q_CTRL_HALT;

}

static void ccp_queue_write(CcpV5State *s, hwaddr offset, uint32_t val,
                                uint32_t id) {
    CcpV5QState *qs = &s->q_states[id];
    switch(offset) {
        case CCP_Q_CTRL_OFFSET:
            trace_ccp_queue_ctrl_write(id, val);
            qs->ccp_q_control = val;
            /*TODO: Is this the correct place to start processing the queue? Or should
             * we only start the queue on head/tail writes? */
            //if (qs->ccp_q_control & CCP_Q_RUN) {
            //    /* Process the request in 100ns from now */
            //    if (s->dma_timer_qid == -1) {
            //        s->dma_timer_qid = id;
            //        timer_mod(&s->dma_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 100);
            //    } else {
            //        qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Can't program timer. A CCP request is already pending.\n");
            //    }
            //}
            break;
        case CCP_Q_TAIL_LO_OFFSET:
            qs->ccp_q_tail = val;
            trace_ccp_queue_tail_lo_write(id, val);
            break;
        case CCP_Q_HEAD_LO_OFFSET:
            qs->ccp_q_head = val;
            trace_ccp_queue_head_lo_write(id, val);
            break;
        case CCP_Q_STATUS_OFFSET:
            qs->ccp_q_status = val;
            trace_ccp_queue_status_write(id, val);
            break;
        default:
            qemu_log_mask(LOG_UNIMP, "CCP: CCP queue write at unknown " \
                          "offset: 0x%" HWADDR_PRIx " val 0x%x\n", offset, val);
    }
    return;
}

static uint32_t ccp_ctrl_read(CcpV5State *s, hwaddr offset) {
    (void)s;

    /* TODO: handle or log global register access */
    switch (offset) {
        case CCP_G_Q_MASK_OFFSET:
            /* Ignored for now */
            break;
        case CCP_G_Q_PRIO_OFFSET:
            /* Ignored for now */
            break;
        default:
            break;
    }

    trace_ccp_queue_ctrl_read(offset, 0);
    return 0;
}

static void ccp_ctrl_write(CcpV5State *s, hwaddr offset,
                                    uint32_t value) {
    (void)s;
    /* TODO: handle or log global register access */
    switch (offset) {
        case CCP_G_Q_MASK_OFFSET:
            /* Ignored for now */
            break;
        case CCP_G_Q_PRIO_OFFSET:
            /* Ignored for now */
            break;
        default:
            break;
    }

    trace_ccp_ctrl_write(offset, value);
    return;
}

static uint32_t ccp_config_read(CcpV5State *s, hwaddr offset) {
    (void)s;

    uint32_t val = 0;
    switch (offset) {
        case 0x38:
            /* The on chip bootloader waits for bit 0 to go 1. */
            val = 1;
            break;
    }

    trace_ccp_config_read(offset, val);
    return val;
}

static void ccp_config_write(CcpV5State *s, hwaddr offset,
                                    uint32_t value) {
    (void)s;
    trace_ccp_config_write(offset, value);
    return;
}

static uint64_t ccp_read(void *opaque, hwaddr offset, unsigned int size) {
    CcpV5State *s = CCP_V5(opaque);
    uint32_t ret = 0;
    uint32_t id = 0;

    /* qemu_log_mask(LOG_UNIMP, "CCP: MMIO read at 0x%" HWADDR_PRIx "\n", */
    /*               offset); */

    if (size != sizeof(uint32_t)) {
        qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Unsupported read size: " \
                                       "0x%x\n", size);
        return 0;
    }
    if (offset < CCP_Q_OFFSET) {
        ret = ccp_ctrl_read(s,offset);
    } else if (offset < CCP_CONFIG_OFFSET) {
        /* CCP queue access */
        if (id > CCP_Q_COUNT) {
            /* This should never happen, but... */
            qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Invalid queue id 0x%d\n",
                          id);
            return ret;
        }
        id = (offset >> 12) - 1; // Make queue index start with 0
        ret = ccp_queue_read(s, offset & 0xfff, id);
    } else {
        ret = ccp_config_read(s,offset & 0xfff);
    }

    return ret;
}

static void ccp_write(void *opaque, hwaddr offset,
                       uint64_t value, unsigned int size) {
    CcpV5State *s = CCP_V5(opaque);
    uint32_t id = 0;

    if (size != sizeof(uint32_t)) {
        qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Unsupported write size:" \
                                       "0x%x\n", size);
        return;
    }

    if (offset < CCP_Q_OFFSET) {
        /* TODO: Explicit cast? */
        ccp_ctrl_write(s, offset, value);
    } else if (offset < CCP_CONFIG_OFFSET) {
        /* CCP queue access */
        if (id > CCP_Q_COUNT) {
            /* This should never happen, but... */
            qemu_log_mask(LOG_GUEST_ERROR, "CCP Error: Invalid queue id 0x%d\n",
                          id);
            return;
        }
        id = (offset >> 12) - 1; // Make queue index start with 0
        ccp_queue_write(s, offset & 0xfff, value, id);
    } else {
        ccp_config_write(s,offset & 0xfff, value);
    }

}

static void ccp_init_q(CcpV5State *s) {
    int i;

    for (i = 0; i < CCP_Q_COUNT; i++) {
        memset(&s->q_states[i], 0, sizeof(CcpV5QState));
        s->q_states[i].ccp_q_control = CCP_Q_CTRL_HALT;
        s->q_states[i].ccp_q_status = CCP_Q_STATUS_SUCCESS;
        s->q_states[i].ccp_q_id = i;
    }

}

static const MemoryRegionOps ccp_mem_ops = {
    .read = ccp_read,
    .write = ccp_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    /* TODO: Does the CCP really allow non-word aligned accesses ? */
    .impl.min_access_size = 1,
    .impl.max_access_size = 4,
};

static void ccp_init(Object *obj) {
    CcpV5State *s = CCP_V5(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    //timer_init_ns(&s->dma_timer, QEMU_CLOCK_VIRTUAL, ccp_timer_cb, s);
    //s->dma_timer_qid = -1;

    memory_region_init_io(&s->iomem, obj, &ccp_mem_ops, s,
            TYPE_CCP_V5, CCP_MMIO_SIZE);

    sysbus_init_mmio(sbd, &s->iomem);

    ccp_init_q(s);
    s->sha_ctx.raw = NULL;

}

static const TypeInfo ccp_info = {
    .name = TYPE_CCP_V5,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_init = ccp_init,
    .instance_size = sizeof(CcpV5State),

};

static void ccp_register_types(void) {
    type_register_static(&ccp_info);

}

type_init(ccp_register_types);
