

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h> 
#include <time.h>

static inline uint32_t rotl32(uint32_t x, int n){ return (x<<n) | (x>>(32-n)); }
static inline uint32_t bswap32(uint32_t x){
    return ((x & 0x000000FFu) << 24) |
           ((x & 0x0000FF00u) << 8)  |
           ((x & 0x00FF0000u) >> 8)  |
           ((x & 0xFF000000u) >> 24);
}
static inline uint32_t load_be32(const void* p){
    uint32_t x; memcpy(&x,p,4); return bswap32(x);
}
static inline void store_be32(void* p, uint32_t x){
    x = bswap32(x); memcpy(p,&x,4);
}

static const uint32_t FK[4] = {
    0xA3B1BAC6u, 0x56AA3350u, 0x677D9197u, 0xB27022DCu
};
static const uint32_t CK[32] = {
    0x00070E15u,0x1C232A31u,0x383F464Du,0x545B6269u,
    0x70777E85u,0x8C939AA1u,0xA8AFB6BDu,0xC4CBD2D9u,
    0xE0E7EEF5u,0xFC030A11u,0x181F262Du,0x343B4249u,
    0x50575E65u,0x6C737A81u,0x888F969Du,0xA4ABB2B9u,
    0xC0C7CED5u,0xDCE3EAF1u,0xF8FF060Du,0x141B2229u,
    0x30373E45u,0x4C535A61u,0x686F767Du,0x848B9299u,
    0xA0A7AEB5u,0xBCC3CAD1u,0xD8DFE6EDu,0xF4FB0209u,
    0x10171E25u,0x2C333A41u,0x484F565Du,0x646B7279u
};
static const uint8_t SBOX[256] = {
    0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05,
    0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99,
    0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62,
    0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6,
    0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8,
    0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35,
    0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87,
    0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e,
    0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1,
    0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3,
    0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f,
    0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51,
    0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8,
    0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0,
    0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84,
    0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48
};

// ----------------------------- T-table -----------------------------
static uint32_t T0[256], T1[256], T2[256], T3[256];
static inline uint32_t L32(uint32_t x){
    return x ^ rotl32(x,2) ^ rotl32(x,10) ^ rotl32(x,18) ^ rotl32(x,24);
}
static void build_Ttables(){
    for(int b=0;b<256;b++){
        uint8_t s = SBOX[b];
        uint32_t x = ((uint32_t)s) << 24;
        uint32_t t = L32(x);
        T0[b] = t;
        T1[b] = rotl32(t,8);
        T2[b] = rotl32(t,16);
        T3[b] = rotl32(t,24);
    }
}

typedef struct {
    uint32_t rk[32];
} sm4_key_t;

static inline uint32_t tau32(uint32_t a){
    uint8_t b0 = SBOX[(a>>24)&0xFF];
    uint8_t b1 = SBOX[(a>>16)&0xFF];
    uint8_t b2 = SBOX[(a>> 8)&0xFF];
    uint8_t b3 = SBOX[(a    )&0xFF];
    return ((uint32_t)b0<<24)|((uint32_t)b1<<16)|((uint32_t)b2<<8)|b3;
}
static inline uint32_t Lp32(uint32_t x){
    return x ^ rotl32(x,13) ^ rotl32(x,23);
}

void sm4_key_schedule(const uint8_t key[16], sm4_key_t *ks){
    uint32_t K[4];
    K[0] = load_be32(key+0) ^ FK[0];
    K[1] = load_be32(key+4) ^ FK[1];
    K[2] = load_be32(key+8) ^ FK[2];
    K[3] = load_be32(key+12)^ FK[3];
    for(int i=0;i<32;i++){
        uint32_t t = K[1] ^ K[2] ^ K[3] ^ CK[i];
        t = tau32(t);
        t = Lp32(t);
        uint32_t rk = K[0] ^ t;
        ks->rk[i] = rk;
        K[0]=K[1]; K[1]=K[2]; K[2]=K[3]; K[3]=rk;
    }
}


static inline __m256i gather32_from_table(const uint32_t *tbl, __m256i idx_bytes){
    // idx_bytes contains 8 lanes each with 0..255; need to scale to byte offsets (4 bytes each)
    __m256i index = _mm256_slli_epi32(idx_bytes, 2); // *4 because i32gather takes byte offset
    return _mm256_i32gather_epi32((const int*)tbl, index, 1);
}

static inline void sm4_encrypt8_ecb(const sm4_key_t* ks, const uint8_t inblk[8][16], uint8_t outblk[8][16]){
    uint32_t x0[8], x1[8], x2[8], x3[8];
    for(int i=0;i<8;i++){
        x0[i] = load_be32(inblk[i]+0);
        x1[i] = load_be32(inblk[i]+4);
        x2[i] = load_be32(inblk[i]+8);
        x3[i] = load_be32(inblk[i]+12);
    }
    __m256i X0 = _mm256_loadu_si256((const __m256i*)x0);
    __m256i X1 = _mm256_loadu_si256((const __m256i*)x1);
    __m256i X2 = _mm256_loadu_si256((const __m256i*)x2);
    __m256i X3 = _mm256_loadu_si256((const __m256i*)x3);

    for(int r=0;r<32;r++){
        __m256i rk = _mm256_set1_epi32((int)ks->rk[r]);
        __m256i t = _mm256_xor_si256(_mm256_xor_si256(X1, X2), _mm256_xor_si256(X3, rk));
        __m256i b0 = _mm256_and_si256(t, _mm256_set1_epi32(0xFF));
        __m256i b1 = _mm256_and_si256(_mm256_srli_epi32(t,8), _mm256_set1_epi32(0xFF));
        __m256i b2 = _mm256_and_si256(_mm256_srli_epi32(t,16), _mm256_set1_epi32(0xFF));
        __m256i b3 = _mm256_and_si256(_mm256_srli_epi32(t,24), _mm256_set1_epi32(0xFF));
        __m256i y0 = gather32_from_table(T0, b3); 
        __m256i y1 = gather32_from_table(T1, b2);
        __m256i y2 = gather32_from_table(T2, b1);
        __m256i y3 = gather32_from_table(T3, b0);
        __m256i y = _mm256_xor_si256(_mm256_xor_si256(y0,y1), _mm256_xor_si256(y2,y3));
        __m256i Xn = _mm256_xor_si256(X0, y);

        X0 = X1; X1 = X2; X2 = X3; X3 = Xn;
    }
    _mm256_storeu_si256((__m256i*)x0, X3);
    _mm256_storeu_si256((__m256i*)x1, X2);
    _mm256_storeu_si256((__m256i*)x2, X1);
    _mm256_storeu_si256((__m256i*)x3, X0);
    for(int i=0;i<8;i++){
        store_be32(outblk[i]+0, x0[i]); store_be32(outblk[i]+4, x1[i]);
        store_be32(outblk[i]+8, x2[i]); store_be32(outblk[i]+12, x3[i]);
    }
}

static void sm4_encrypt_block_single(const sm4_key_t* ks, const uint8_t in[16], uint8_t out[16]){
    uint8_t inblk[8][16];
    uint8_t outblk[8][16];
    for(int i=0;i<8;i++) memcpy(inblk[i], in, 16);
    sm4_encrypt8_ecb(ks, inblk, outblk);
    memcpy(out, outblk[0], 16);
}


static void gf128_xor(uint8_t z[16], const uint8_t a[16]){
    for(int i=0;i<16;i++) z[i] ^= a[i];
}

static void gf128_shift_right_one(uint8_t a[16]){
    uint8_t carry = 0;
    for(int i=0;i<16;i++){
        uint8_t newcarry = a[i] & 1;
        a[i] = (a[i] >> 1) | (carry ? 0x80 : 0x00);
        carry = newcarry;
    }
}

static void gf128_mul_bitwise(const uint8_t X[16], const uint8_t Y[16], uint8_t Z[16]){
    uint8_t V[16]; memcpy(V, Y, 16);
    uint8_t acc[16] = {0};
    for(int i=0;i<128;i++){
        // check bit i of X (from MSB to LSB)
        int byte_idx = i >> 3; // 0..15
        int bit_in_byte = 7 - (i & 7);
        if( (X[byte_idx] >> bit_in_byte) & 1 ){
            // acc ^= V
            for(int j=0;j<16;j++) acc[j] ^= V[j];
        }
        uint8_t lsb = V[15] & 1;
        gf128_shift_right_one(V);
        if(lsb){
            V[0] ^= 0xE1;
        }
    }
    memcpy(Z, acc, 16);
}

typedef struct {
    uint8_t H[16];
    uint8_t Y[16]; // running GHASH value (big-endian bytes)
} ghash_ctx;

void ghash_init(ghash_ctx *ctx, const uint8_t H[16]){
    memcpy(ctx->H, H, 16);
    memset(ctx->Y, 0, 16);
}

void ghash_update_block(ghash_ctx *ctx, const uint8_t block[16]){
    uint8_t tmp[16];
    for(int i=0;i<16;i++) tmp[i] = ctx->Y[i] ^ block[i];
    uint8_t prod[16];
    gf128_mul_bitwise(tmp, ctx->H, prod);
    memcpy(ctx->Y, prod, 16);
}

// process data that might be multiple of 16 bytes
void ghash_update(ghash_ctx *ctx, const uint8_t *data, size_t len){
    while(len >= 16){
        ghash_update_block(ctx, data);
        data += 16; len -= 16;
    }
    if(len){
        uint8_t last[16] = {0};
        memcpy(last, data, len);
        ghash_update_block(ctx, last);
    }
}

void ghash_finish(ghash_ctx *ctx, uint8_t out[16]){
    memcpy(out, ctx->Y, 16);
}

// ----------------------------- GCM----------------------------

static void increment_be128(uint8_t ctr[16]){
    for(int i=15;i>=0;i--){
        if(++ctr[i]) break;
    }
}

void sm4_gcm_init_iv(const sm4_key_t *ks, const uint8_t iv[], size_t iv_len, uint8_t J0[16]){
    if(iv_len == 12){
        memcpy(J0, iv, 12);
        J0[12]=0; J0[13]=0; J0[14]=0; J0[15]=1;
    }else{
        uint8_t zero[16] = {0};
        uint8_t H[16];
        sm4_encrypt_block_single(ks, zero, H);
        ghash_ctx gctx;
        ghash_init(&gctx, H);
        ghash_update(&gctx, iv, iv_len);
        uint8_t lenblock[16] = {0};
        uint64_t ivbits = (uint64_t)iv_len * 8;
        for(int i=0;i<8;i++) lenblock[8 + i] = (uint8_t)( (ivbits >> (56 - 8*i)) & 0xFF );
        ghash_update_block(&gctx, lenblock);
        ghash_finish(&gctx, J0);
    }
}

int sm4_gcm_encrypt_and_tag(const sm4_key_t *ks,
                            const uint8_t iv[], size_t iv_len,
                            const uint8_t *aad, size_t aad_len,
                            const uint8_t *plaintext, size_t pt_len,
                            uint8_t *ciphertext,
                            uint8_t *tag, size_t tag_len){
    if(tag_len > 16) return -1;
    uint8_t zero[16] = {0};
    uint8_t H[16];
    sm4_encrypt_block_single(ks, zero, H);

    uint8_t J0[16];
    sm4_gcm_init_iv(ks, iv, iv_len, J0);

    ghash_ctx gctx;
    ghash_init(&gctx, H);

    if(aad && aad_len) ghash_update(&gctx, aad, aad_len);


    uint8_t ctr[16];
    memcpy(ctr, J0, 16);
    size_t offset = 0;
    while(pt_len >= 16 * 8){
        // build 8 ctr blocks
        uint8_t inctr[8][16];
        for(int i=0;i<8;i++){
            memcpy(inctr[i], ctr, 16);
            increment_be128(ctr);
        }
        uint8_t keystream[8][16];
        sm4_encrypt8_ecb(ks, inctr, keystream);
        for(int i=0;i<8;i++){
            const uint8_t *p = plaintext + offset + i*16;
            uint8_t *c = ciphertext + offset + i*16;
            for(int b=0;b<16;b++) c[b] = p[b] ^ keystream[i][b];
            ghash_update_block(&gctx, c);
        }
        offset += 16*8;
        pt_len -= 16*8;
    }
    while(pt_len >= 16){
        uint8_t ksblk[16];
        sm4_encrypt_block_single(ks, ctr, ksblk);
        increment_be128(ctr);
        const uint8_t *p = plaintext + offset;
        uint8_t *c = ciphertext + offset;
        for(int b=0;b<16;b++) c[b] = p[b] ^ ksblk[b];
        ghash_update_block(&gctx, c);
        offset += 16; pt_len -= 16;
    }
    if(pt_len){
        uint8_t ksblk[16];
        sm4_encrypt_block_single(ks, ctr, ksblk);
        increment_be128(ctr);
        uint8_t ctail[16] = {0};
        for(size_t i=0;i<pt_len;i++){
            ctail[i] = plaintext[offset + i] ^ ksblk[i];
            ciphertext[offset + i] = ctail[i];
        }
        ghash_update_block(&gctx, ctail);
    }

    uint8_t lenblock[16] = {0};
    uint64_t aadbits = (uint64_t)aad_len * 8;
    uint64_t ctbbits = 0; 
    (void)ctb; 
    uint64_t ctb = (uint64_t)( (size_t)( ( (offset) ) ) ) * 8; 
    for(int i=0;i<8;i++) lenblock[i] = (uint8_t)( (aadbits >> (56 - 8*i)) & 0xFF );
    for(int i=0;i<8;i++) lenblock[8+i] = (uint8_t)( (ctb   >> (56 - 8*i)) & 0xFF );
    ghash_update_block(&gctx, lenblock);

    uint8_t S[16];
    sm4_encrypt_block_single(ks, J0, S);
    uint8_t GH[16];
    ghash_finish(&gctx, GH);
    uint8_t T[16];
    for(int i=0;i<16;i++) T[i] = S[i] ^ GH[i];
    memcpy(tag, T, tag_len);
    return 0;
}


int selftest(){
    uint8_t key[16];
    for(int i=0;i<16;i++) key[i] = i;
    sm4_key_t ks; sm4_key_schedule(key, &ks);
    build_Ttables(); 

    uint8_t iv[12] = {0};
    uint8_t aad[20] = {0x01,0x02,0x03};
    uint8_t pt[64];
    for(int i=0;i<64;i++) pt[i] = (uint8_t)i;
    uint8_t ct[64];
    uint8_t tag[16];

    if(sm4_gcm_encrypt_and_tag(&ks, iv, 12, aad, 3, pt, 64, ct, tag, 16) != 0){
        printf("Encrypt error"); 
        return 0;
    }
    printf("Ciphertext (first 32 bytes):");
    for(int i=0;i<32;i++) printf("%02x\", ct[i]); printf("Tag: "); for(int i=0;i<16;i++) printf("%02x\", tag[i]); printf("");
    return 1;
}

int main(){
    if(!selftest()) return 1;
    printf("SM4-GCM selftest done.");
    return 0;
}

