#include <stdio.h>
#include <string.h>

#define uint8_t unsigned char
#define uint32_t unsigned int
#define uint16_t unsigned short
//{ 
// x^8 + x^7 + x^4 + x^3 + x + 1    
//#define POLY_LEN 8
//#define POLY 0x9B // x^8 + x^7 + x^4 + x^3 + x + 1    
//#define POLY_INIT 0xFF // x^8 + x^7 + x^4 + x^3 + x + 1    
//#define POLY_MASK ((1<<POLY_LEN) - 1) // clear useless bit    
//#define POLY_MSB (1<<(POLY_LEN-1)) // MSB bit according to POLY_LEN
//}

//{ 
// x^8 + x^2 + x + 1    
//#define POLY_LEN 8
//#define POLY 0x07 // x^8 + x^2 + x + 1    
//#define POLY_INIT 0x00 // x^8 + x^2 + x + 1    
//#define POLY_MASK ((1<<POLY_LEN) - 1) // clear useless bit    
//#define POLY_MSB (1<<(POLY_LEN-1)) // MSB bit according to POLY_LEN
//}

//{
//x10+x3+1
//#define POLY_LEN 10
//#define POLY 0x09 // 
//#define POLY_INIT 0x3FF // 
//#define POLY_MASK ((1<<POLY_LEN) - 1) // clear useless bit    
//#define POLY_MSB (1<<(POLY_LEN-1)) // MSB bit according to POLY_LEN
//}


//{
//x16+x5+x3+x2+1
#define POLY_LEN 16
#define POLY 0x1D // 
#define POLY_INIT 0xFFFF // 
#define POLY_MASK ((1<<POLY_LEN) - 1) // clear useless bit    
#define POLY_MSB (1<<(POLY_LEN-1)) // MSB bit according to POLY_LEN
//}


// verify crc code through https://crccalc.com/
#define MAX_DATA_BYTE_LEN 256
#define CRC_PARARREL_BYTE_LEN 4
#define MAX_CRC_PARARREL_BLOCK_NUM (MAX_DATA_BYTE_LEN / CRC_PARARREL_BYTE_LEN)
static uint8_t M[POLY_LEN][POLY_LEN] = { { 0 } };

// calculate byte, iterate each bit
uint32_t _crc_calc_byte (uint32_t crc_in, uint8_t data_in) {
    uint32_t   i;
    uint32_t   data_shift;
    uint32_t   data;
    data_shift = data_in << (POLY_LEN - 8); 
    data = ( crc_in ^ data_shift ) & POLY_MASK;

    for ( i = 0; i < 8; i++ )
    {
        if ( ( data & POLY_MSB ) != 0 )
        {
            data <<= 1;
            data ^= POLY;
        }
        else
        {
            data <<= 1;
        }
    }
    return data & POLY_MASK;
}

uint32_t crc_calc_standard(uint32_t crc_default, uint8_t *data_input, uint32_t data_byte_len) {
    uint32_t crc = crc_default;
    for (uint32_t i = 0; i < data_byte_len; i++) {
        crc = _crc_calc_byte(crc, data_input[i]);
    }
    return crc & POLY_MASK;

}
// init crc vec for given data_pararrel_byte_length(each calculation block data length)
int crc_vec_init(int data_pararrel_byte_length) {
    int data_pararrel_bit_length = data_pararrel_byte_length << 3;
    // init l = 0 case;
    for ( int i = 0; i < POLY_LEN; i++ ) {
        for ( int k = 0; k < POLY_LEN; k++ ) {
            M[i][k] = (i == k) ? 1 : 0;
        }
    }

    // build
    for ( int l = 1; l <= data_pararrel_bit_length; l++  ) {
        for ( int k = 0; k < POLY_LEN; k++ ) {
            uint8_t sign = M[POLY_LEN-1][k]; 
            for ( int i = POLY_LEN-1; i >= 0; i-- ) {
                if ( i == 0 ) {
                    M[i][k] = sign;
                } else {
                    M[i][k] = (POLY&(1<<i)) ? ( M[i-1][k] ^ sign ) : M[i-1][k];
                }
            }
        }
    } 

    printf("M[i][k]:\n");
    for ( int i = POLY_LEN; i >= 0; i-- ) {
        if ( i == POLY_LEN ) {
            printf("|  |");
        } else {
            printf("%2d|",i);
        }
    }
    printf("\n");
    for ( int i = POLY_LEN-1; i >= 0; i-- ) {
        printf("|%2d|", i);
        for ( int k = POLY_LEN-1; k >= 0; k-- ) {
            printf("%2d|", M[i][k]);
        }
        printf("\n");
    }
}

uint32_t crc_calc_parallel (uint32_t crc_default, uint8_t *data_input, uint32_t data_byte_len) {
    uint32_t parrallel_block_num = data_byte_len / CRC_PARARREL_BYTE_LEN;
    uint32_t crc_pararrel_result[MAX_CRC_PARARREL_BLOCK_NUM] ;
    uint8_t *data_in;
    uint32_t b = 0;
    uint32_t a = 0;
    uint32_t A[POLY_LEN] = { 0 };
    uint32_t B[POLY_LEN] = { 0 };
    uint32_t byte_calc = 0;
    uint32_t data_in_len = 0;
    uint32_t pararrel_index = 0;

    if ( 0 != ( data_byte_len % CRC_PARARREL_BYTE_LEN ) ) {
        printf("[WARNING] data_byte_len[%d] MOD data_pararrel_len[%d] != 0\n", data_byte_len, CRC_PARARREL_BYTE_LEN);
        return 0;
    }

    printf("data_byte_len = %d, parrallel_block_num = %d\n", data_byte_len, parrallel_block_num);
    if ( parrallel_block_num > MAX_CRC_PARARREL_BLOCK_NUM ) {
        printf("FATAL ERROR: block_bum_needed(%d) > block_bum_max=(%d)\n", parrallel_block_num, MAX_CRC_PARARREL_BLOCK_NUM);
        return 0;
    }

    while ( byte_calc < data_byte_len ) {
        crc_default = ( 0 == byte_calc ) ? crc_default : 0;
        data_in = data_input + byte_calc;
        if ( data_byte_len - byte_calc > CRC_PARARREL_BYTE_LEN ) {
            data_in_len = CRC_PARARREL_BYTE_LEN;
        } else {
            data_in_len = data_byte_len - byte_calc;
        }
        crc_pararrel_result[pararrel_index] = crc_calc_standard(crc_default, data_in, data_in_len); 
        byte_calc += data_in_len; 
        pararrel_index++;
    }

    b = 0;// b of first block is 0
    for (int j = 0; j < parrallel_block_num; j++) {
        a = crc_pararrel_result[j] ^ b;
        for ( int k = 0; k < POLY_LEN; k++) {
            A[k] = (a>>k) & 0x1;
        }
        for ( int i = 0; i < POLY_LEN; i++) {
            B[i] = 0;
            for ( int k = 0; k < POLY_LEN; k++) {
                B[i] = ( A[k] & M[i][k] ) ^ B[i];
            }
        }
        b = 0;
        for ( int i = 0; i < POLY_LEN; i++) {
            b = (( B[i] << i ) & (1<<i) ) | b;
        }
    }
    return a;
}
int main() {
    uint8_t data[MAX_DATA_BYTE_LEN] = { 0 };
    uint32_t crc = POLY_INIT;
    uint32_t crc_standard = 0;
    uint32_t crc_pararrel = 0;
    uint32_t byte_length = 0;

    crc_vec_init(CRC_PARARREL_BYTE_LEN);

    printf ("verify crc code through https://crccalc.com/ or http://www.ip33.com/crc.html\n");
    printf("input data(exit for terminate):");
    scanf("%s", data);
    while ( strcmp(data, "exit") != 0 ) {
        crc = POLY_INIT;
        byte_length = strlen(data);
        crc_standard = crc_calc_standard(crc, data, byte_length);

        crc = POLY_INIT;
        byte_length = strlen(data);
        if ( 0 == ( byte_length % CRC_PARARREL_BYTE_LEN ) ) {
            crc_pararrel = crc_calc_parallel(crc, data, byte_length);
            printf("poly=0x%x, init=0x%x, data = %s, standard_crc=0x%x, pararrel_crc = 0x%x\n", 
                    POLY, POLY_INIT, data, crc_standard, crc_pararrel);
            if ( crc_standard == crc_pararrel ) {
                printf("[INFO] crc check pass\n");
            } else {
                printf("[ERROR] crc check fail\n");
            }
        } else {
            printf("[WARNING]: data_byte_len[%d] Mod data_pararrel_len[%d] != 0\n", byte_length, CRC_PARARREL_BYTE_LEN);
        }
        printf("\n"); 
        printf("input data(exit for terminate):");
        scanf("%s", data);
    }

    return 0;
}
