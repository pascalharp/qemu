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
/* Copied from Linux sources: drivers/crypto/ccp/ccp-dev-v5.c,  
 * drivers/crypto/ccp/ccp-dev.h and include/linux/ccp.h
 */

/***** AES engine *****/
/**
 * ccp_aes_type - AES key size
 *
 * @CCP_AES_TYPE_128: 128-bit key
 * @CCP_AES_TYPE_192: 192-bit key
 * @CCP_AES_TYPE_256: 256-bit key
 */
enum ccp_aes_type {
	CCP_AES_TYPE_128 = 0,
	CCP_AES_TYPE_192,
	CCP_AES_TYPE_256,
	CCP_AES_TYPE__LAST,
};

/**
 * ccp_aes_mode - AES operation mode
 *
 * @CCP_AES_MODE_ECB: ECB mode
 * @CCP_AES_MODE_CBC: CBC mode
 * @CCP_AES_MODE_OFB: OFB mode
 * @CCP_AES_MODE_CFB: CFB mode
 * @CCP_AES_MODE_CTR: CTR mode
 * @CCP_AES_MODE_CMAC: CMAC mode
 */
enum ccp_aes_mode {
	CCP_AES_MODE_ECB = 0,
	CCP_AES_MODE_CBC,
	CCP_AES_MODE_OFB,
	CCP_AES_MODE_CFB,
	CCP_AES_MODE_CTR,
	CCP_AES_MODE_CMAC,
	CCP_AES_MODE_GHASH,
	CCP_AES_MODE_GCTR,
	CCP_AES_MODE_GCM,
	CCP_AES_MODE_GMAC,
	CCP_AES_MODE__LAST,
};

/**
 * ccp_aes_mode - AES operation mode
 *
 * @CCP_AES_ACTION_DECRYPT: AES decrypt operation
 * @CCP_AES_ACTION_ENCRYPT: AES encrypt operation
 */
enum ccp_aes_action {
	CCP_AES_ACTION_DECRYPT = 0,
	CCP_AES_ACTION_ENCRYPT,
	CCP_AES_ACTION__LAST,
};
/* Overloaded field */
#define	CCP_AES_GHASHAAD	CCP_AES_ACTION_DECRYPT
#define	CCP_AES_GHASHFINAL	CCP_AES_ACTION_ENCRYPT

/***** SHA engine *****/
/**
 * ccp_sha_type - type of SHA operation
 *
 * @CCP_SHA_TYPE_1: SHA-1 operation
 * @CCP_SHA_TYPE_224: SHA-224 operation
 * @CCP_SHA_TYPE_256: SHA-256 operation
 */
enum ccp_sha_type {
	CCP_SHA_TYPE_1 = 1,
	CCP_SHA_TYPE_224,
	CCP_SHA_TYPE_256,
	CCP_SHA_TYPE_384,
	CCP_SHA_TYPE_512,
	CCP_SHA_TYPE__LAST,
};

/***** XTS-AES engine *****/
/**
 * ccp_xts_aes_unit_size - XTS unit size
 *
 * @CCP_XTS_AES_UNIT_SIZE_16: Unit size of 16 bytes
 * @CCP_XTS_AES_UNIT_SIZE_512: Unit size of 512 bytes
 * @CCP_XTS_AES_UNIT_SIZE_1024: Unit size of 1024 bytes
 * @CCP_XTS_AES_UNIT_SIZE_2048: Unit size of 2048 bytes
 * @CCP_XTS_AES_UNIT_SIZE_4096: Unit size of 4096 bytes
 */
enum ccp_xts_aes_unit_size {
	CCP_XTS_AES_UNIT_SIZE_16 = 0,
	CCP_XTS_AES_UNIT_SIZE_512,
	CCP_XTS_AES_UNIT_SIZE_1024,
	CCP_XTS_AES_UNIT_SIZE_2048,
	CCP_XTS_AES_UNIT_SIZE_4096,
	CCP_XTS_AES_UNIT_SIZE__LAST,
};

typedef enum ccp_memtype {
	CCP_MEMTYPE_SYSTEM = 0,
	CCP_MEMTYPE_SB,
	CCP_MEMTYPE_LOCAL,
	CCP_MEMTYPE__LAST,
} ccp_memtype;

/***** Passthru engine *****/
/**
 * ccp_passthru_bitwise - type of bitwise passthru operation
 *
 * @CCP_PASSTHRU_BITWISE_NOOP: no bitwise operation performed
 * @CCP_PASSTHRU_BITWISE_AND: perform bitwise AND of src with mask
 * @CCP_PASSTHRU_BITWISE_OR: perform bitwise OR of src with mask
 * @CCP_PASSTHRU_BITWISE_XOR: perform bitwise XOR of src with mask
 * @CCP_PASSTHRU_BITWISE_MASK: overwrite with mask
 */
typedef enum ccp_passthru_bitwise {
	CCP_PASSTHRU_BITWISE_NOOP = 0,
	CCP_PASSTHRU_BITWISE_AND,
	CCP_PASSTHRU_BITWISE_OR,
	CCP_PASSTHRU_BITWISE_XOR,
	CCP_PASSTHRU_BITWISE_MASK,
	CCP_PASSTHRU_BITWISE__LAST,
} ccp_pt_bitwise;

/**
 * ccp_passthru_byteswap - type of byteswap passthru operation
 *
 * @CCP_PASSTHRU_BYTESWAP_NOOP: no byte swapping performed
 * @CCP_PASSTHRU_BYTESWAP_32BIT: swap bytes within 32-bit words
 * @CCP_PASSTHRU_BYTESWAP_256BIT: swap bytes within 256-bit words
 */
typedef enum ccp_passthru_byteswap {
	CCP_PASSTHRU_BYTESWAP_NOOP = 0,
	CCP_PASSTHRU_BYTESWAP_32BIT,
	CCP_PASSTHRU_BYTESWAP_256BIT,
	CCP_PASSTHRU_BYTESWAP__LAST,
} ccp_pt_byteswap;

/**
 * ccp_engine - CCP operation identifiers
 *
 * @CCP_ENGINE_AES: AES operation
 * @CCP_ENGINE_XTS_AES: 128-bit XTS AES operation
 * @CCP_ENGINE_RSVD1: unused
 * @CCP_ENGINE_SHA: SHA operation
 * @CCP_ENGINE_RSA: RSA operation
 * @CCP_ENGINE_PASSTHRU: pass-through operation
 * @CCP_ENGINE_ZLIB_DECOMPRESS: unused
 * @CCP_ENGINE_ECC: ECC operation
 */
typedef enum ccp_engine {
	CCP_ENGINE_AES = 0,
	CCP_ENGINE_XTS_AES_128,
	CCP_ENGINE_DES3,
	CCP_ENGINE_SHA,
	CCP_ENGINE_RSA,
	CCP_ENGINE_PASSTHRU,
	CCP_ENGINE_ZLIB_DECOMPRESS,
	CCP_ENGINE_ECC,
	CCP_ENGINE__LAST,
} ccp_engine;

/* CCP version 5: Union to define the function field (cmd_reg1/dword0) */
typedef union ccp_function {
	struct {
		uint16_t size:7;
		uint16_t encrypt:1;
		uint16_t mode:5;
		uint16_t type:2;
	} aes;
	struct {
		uint16_t size:7;
		uint16_t encrypt:1;
		uint16_t rsvd:5;
		uint16_t type:2;
	} aes_xts;
	struct {
		uint16_t size:7;
		uint16_t encrypt:1;
		uint16_t mode:5;
		uint16_t type:2;
	} des3;
	struct {
		uint16_t rsvd1:10;
		uint16_t type:4;
		uint16_t rsvd2:1;
	} sha;
	struct {
		uint16_t mode:3;
		uint16_t size:12;
	} rsa;
	struct {
		uint16_t byteswap:2;
		uint16_t bitwise:3;
		uint16_t reflect:2;
		uint16_t rsvd:8;
	} pt;
	struct  {
		uint16_t rsvd:13;
	} zlib;
	struct {
		uint16_t size:10;
		uint16_t type:2;
		uint16_t mode:3;
	} ecc;
	uint16_t raw;
} ccp_function;

#define	CCP_AES_SIZE(p)		((p)->aes.size)
#define	CCP_AES_ENCRYPT(p)	((p)->aes.encrypt)
#define	CCP_AES_MODE(p)		((p)->aes.mode)
#define	CCP_AES_TYPE(p)		((p)->aes.type)
#define	CCP_XTS_SIZE(p)		((p)->aes_xts.size)
#define	CCP_XTS_TYPE(p)		((p)->aes_xts.type)
#define	CCP_XTS_ENCRYPT(p)	((p)->aes_xts.encrypt)
#define	CCP_DES3_SIZE(p)	((p)->des3.size)
#define	CCP_DES3_ENCRYPT(p)	((p)->des3.encrypt)
#define	CCP_DES3_MODE(p)	((p)->des3.mode)
#define	CCP_DES3_TYPE(p)	((p)->des3.type)
#define	CCP_SHA_TYPE(p)		((p)->sha.type)
#define	CCP_RSA_SIZE(p)		((p)->rsa.size)
#define	CCP_PT_BYTESWAP(p)	((p)->pt.byteswap)
#define	CCP_PT_BITWISE(p)	((p)->pt.bitwise)
#define	CCP_ECC_MODE(p)		((p)->ecc.mode)
#define	CCP_ECC_AFFINE(p)	((p)->ecc.one)

/* Word 0 */
#define CCP5_CMD_DW0(p)		((p)->dw0)
#define CCP5_CMD_SOC(p)		(CCP5_CMD_DW0(p).soc)
#define CCP5_CMD_IOC(p)		(CCP5_CMD_DW0(p).ioc)
#define CCP5_CMD_INIT(p)	(CCP5_CMD_DW0(p).init)
#define CCP5_CMD_EOM(p)		(CCP5_CMD_DW0(p).eom)
#define CCP5_CMD_FUNCTION(p)	(CCP5_CMD_DW0(p).function)
#define CCP5_CMD_ENGINE(p)	(CCP5_CMD_DW0(p).engine)
#define CCP5_CMD_PROT(p)	(CCP5_CMD_DW0(p).prot)

/* Word 1 */
#define CCP5_CMD_DW1(p)		((p)->length)
#define CCP5_CMD_LEN(p)		(CCP5_CMD_DW1(p))

/* Word 2 */
#define CCP5_CMD_DW2(p)		((p)->src_lo)
#define CCP5_CMD_SRC_LO(p)	(CCP5_CMD_DW2(p))

/* Word 3 */
#define CCP5_CMD_DW3(p)		((p)->dw3)
#define CCP5_CMD_SRC_MEM(p)	((p)->dw3.src_mem)
#define CCP5_CMD_SRC_HI(p)	((p)->dw3.src_hi)
#define CCP5_CMD_LSB_ID(p)	((p)->dw3.lsb_cxt_id)
#define CCP5_CMD_FIX_SRC(p)	((p)->dw3.fixed)

/* Words 4/5 */
#define CCP5_CMD_DW4(p)		((p)->dw4)
#define CCP5_CMD_DST_LO(p)	(CCP5_CMD_DW4(p).dst_lo)
#define CCP5_CMD_DW5(p)		((p)->dw5.fields.dst_hi)
#define CCP5_CMD_DST_HI(p)	(CCP5_CMD_DW5(p))
#define CCP5_CMD_DST_MEM(p)	((p)->dw5.fields.dst_mem)
#define CCP5_CMD_FIX_DST(p)	((p)->dw5.fields.fixed)
#define CCP5_CMD_SHA_LO(p)	((p)->dw4.sha_len_lo)
#define CCP5_CMD_SHA_HI(p)	((p)->dw5.sha_len_hi)

/* Word 6/7 */
#define CCP5_CMD_DW6(p)		((p)->key_lo)
#define CCP5_CMD_KEY_LO(p)	(CCP5_CMD_DW6(p))
#define CCP5_CMD_DW7(p)		((p)->dw7)
#define CCP5_CMD_KEY_HI(p)	((p)->dw7.key_hi)
#define CCP5_CMD_KEY_MEM(p)	((p)->dw7.key_mem)

/**
 * descriptor for version 5 CPP commands
 * 8 32-bit words:
 * word 0: function; engine; control bits
 * word 1: length of source data
 * word 2: low 32 bits of source pointer
 * word 3: upper 16 bits of source pointer; source memory type
 * word 4: low 32 bits of destination pointer
 * word 5: upper 16 bits of destination pointer; destination memory type
 * word 6: low 32 bits of key pointer
 * word 7: upper 16 bits of key pointer; key memory type
 */
struct dword0 {
	unsigned int soc:1;
	unsigned int ioc:1;
	unsigned int rsvd1:1;
	unsigned int init:1;
	unsigned int eom:1;		/* AES/SHA only */
	unsigned int function:15;
	unsigned int engine:4;
	unsigned int prot:1;
	unsigned int rsvd2:7;
};

struct dword3 {
	unsigned int  src_hi:16;
	unsigned int  src_mem:2;
	unsigned int  lsb_cxt_id:8;
	unsigned int  rsvd1:5;
	unsigned int  fixed:1;
};

union dword4 {
	uint32_t dst_lo;		/* NON-SHA	*/
	uint32_t sha_len_lo;	/* SHA		*/
};

union dword5 {
	struct {
		unsigned int  dst_hi:16;
		unsigned int  dst_mem:2;
		unsigned int  rsvd1:13;
		unsigned int  fixed:1;
	} fields;
	uint32_t sha_len_hi;
};

struct dword7 {
	unsigned int  key_hi:16;
	unsigned int  key_mem:2;
	unsigned int  rsvd1:14;
};

typedef struct ccp5_desc {
	struct dword0 dw0;
	uint32_t length;
	uint32_t src_lo;
	struct dword3 dw3;
	union dword4 dw4;
	union dword5 dw5;
	uint32_t key_lo;
	struct dword7 dw7;
} ccp5_desc;
