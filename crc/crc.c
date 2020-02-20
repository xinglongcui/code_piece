#include <stdio.h>
#include <string.h>

// verify crc code through https://crccalc.com/
#define MAX_DATA_BYTE_LEN 256
#define CRC_PARARREL_BYTE_LEN 2 
#define MAX_CRC_PARARREL_BLOCK_NUM (MAX_DATA_BYTE_LEN / CRC_PARARREL_BYTE_LEN)
#define uint8_t unsigned char
#define uint32_t unsigned int
//#define POLY 0x07 // x^8 + x^2 + x + 1    
//#define INIT 0x00 // x^8 + x^2 + x + 1    

#define POLY 0x9B // x^8 + x^7 + x^4 + x^3 + x + 1    
#define INIT 0xFF // x^8 + x^7 + x^4 + x^3 + x + 1    
#define CRC_LEN 8

static uint8_t M[8][8] = { { 0 } };
uint8_t _crc8_calc_8bit (uint8_t crc_in, uint8_t data_in) {
    uint8_t   i;
    uint8_t   data;

    data = crc_in ^ data_in;

    for ( i = 0; i < 8; i++ )
    {
        if (( data & 0x80 ) != 0 )
        {
            data <<= 1;
            data ^= POLY;
        }
        else
        {
            data <<= 1;
        }
    }
    return data;
}

uint8_t crc8_calc_standard(uint8_t crc_default, uint8_t *data_input, uint32_t data_byte_len) {
    uint8_t crc = crc_default;
    for (int i = 0; i < data_byte_len; i++) {
        crc = _crc8_calc_8bit(crc, data_input[i]);
    }
    return crc;

}
// init crc8 vec for given data_pararrel_byte_length(each calculation block data length)
int crc8_vec_init(int data_pararrel_byte_length) {
    int data_pararrel_bit_length = data_pararrel_byte_length << 3;
    // init l = 0 case;
    for ( int i = 0; i < CRC_LEN; i++ ) {
        for ( int k = 0; k < 8; k++ ) {
            M[i][k] = (i == k) ? 1 : 0;
        }
    }

    // build
    for ( int l = 1; l <= data_pararrel_bit_length; l++  ) {
        for ( int k = 0; k < 8; k++ ) {
            uint8_t sign = M[7][k]; 
            M[7][k] = M[6][k] ^ sign;
            M[6][k] = M[5][k];
            M[5][k] = M[4][k];
            M[4][k] = M[3][k] ^ sign;
            M[3][k] = M[2][k] ^ sign;
            M[2][k] = M[1][k];
            M[1][k] = M[0][k] ^ sign;
            M[0][k] = sign;
        }
    } 

    printf("M[i][k]:\n");
    printf("| |7|6|5|4|3|2|1|0|\n");
    for ( int i = 7; i >= 0; i-- ) {
        printf("|%d|", i);
        for ( int k = 7; k >= 0; k-- ) {
            printf("%d|", M[i][k]);
        }
        printf("\n");
    }


}

uint8_t crc8_calc_parallel (uint8_t crc_default, uint8_t *data_input, uint8_t data_byte_len) {
    uint8_t parrallel_block_num = data_byte_len / CRC_PARARREL_BYTE_LEN;
    uint8_t crc_pararrel_result[MAX_CRC_PARARREL_BLOCK_NUM] ;
    uint8_t *data_in;
    uint8_t b = 0;
    uint8_t a = 0;
    uint8_t A[8] = { 0 };
    uint8_t B[8] = { 0 };
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
        crc_pararrel_result[pararrel_index] = crc8_calc_standard(crc_default, data_in, data_in_len); 
        byte_calc += data_in_len; 
        pararrel_index++;
    }

    b = 0;// b of first block is 0
    for (int j = 0; j < parrallel_block_num; j++) {
        a = crc_pararrel_result[j] ^ b;
        for ( int k = 0; k < CRC_LEN; k++) {
            A[k] = (a>>k) & 0x1;
        }
        for ( int i = 0; i < CRC_LEN; i++) {
            B[i] = 0;
            for ( int k = 0; k < CRC_LEN; k++) {
                B[i] = ( A[k] & M[i][k] ) ^ B[i];
            }
        }
        b = 0;
        for ( int i = 0; i < CRC_LEN; i++) {
            b = (( B[i] << i ) & (1<<i) ) | b;
        }
    }
    return a;
}
int main() {
    unsigned char data[MAX_DATA_BYTE_LEN] = { 0 };
    unsigned char crc = INIT;
    uint32_t crc_standard = 0;
    uint32_t crc_pararrel = 0;
    int byte_length = 256;

    crc8_vec_init(CRC_PARARREL_BYTE_LEN);

    printf ("verify crc code through https://crccalc.com/\n");
    printf("input data(exit for terminate):");
    scanf("%s", data);
    while ( strcmp(data, "exit") != 0 ) {
        crc = INIT;
        byte_length = strlen(data);
        crc_standard = crc8_calc_standard(crc, data, byte_length);
        printf("data = %s, standard_crc8 = 0x%x\n", data, crc_standard);

        crc = INIT;
        byte_length = strlen(data);
        if ( 0 == ( byte_length % CRC_PARARREL_BYTE_LEN ) ) {
            crc_pararrel = crc8_calc_parallel(crc, data, byte_length);
            printf("data = %s, pararrel_crc8 = 0x%x\n", data, crc_pararrel);
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
