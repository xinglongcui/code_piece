#include <stdio.h>
#include <string.h>

// verify crc code through https://crccalc.com/
#define MAX_DATA_BYTE_LEN 256
#define CRC_PARARREL_BYTE_LEN 2 
#define MAX_CRC_PARARREL_BLOCK_NUM 8
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
    printf("calc str = %s\n", data_input);
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
    for ( int l = 1; l < data_pararrel_bit_length; l++  ) {
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

    //dump
    printf("vec[i][k]:\n");
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
    uint8_t crc_pararrel_result[8] ;
    uint8_t *data_in;
    uint8_t b = 0;
    uint8_t a = 0;
    uint8_t A[8] = { 0 };
    uint8_t B[8] = { 0 };

    if ( ( data_byte_len % CRC_PARARREL_BYTE_LEN ) != 0 ) {
        printf("FATAL ERROR: could not use crc pararrel mode: data_len=%d, pararrel_len=%d\n", data_byte_len, CRC_PARARREL_BYTE_LEN);
        return 0;
    }

    if ( parrallel_block_num > MAX_CRC_PARARREL_BLOCK_NUM ) {
        printf("FATAL ERROR: could not use crc pararrel mode: block_bum_needed=%d, block_bum_max=%d\n", parrallel_block_num, MAX_CRC_PARARREL_BLOCK_NUM);
        return 0;
    }

    for (int j = 0; j < parrallel_block_num; j++) {
        crc_default = (j == 0) ? crc_default : 0;
        data_in = data_input + CRC_PARARREL_BYTE_LEN * j;
        crc_pararrel_result[j] = crc8_calc_standard(crc_default, data_in, CRC_PARARREL_BYTE_LEN); 
    }

  
   //vec[i]{7..0} = (( vec[i][7] << 7 ) & 0x80 ) | ((vec[i][6] << 6) & 0x40) | ... | ((vec[i][0] << 0) &0x1); 
   //assgin C[8] = vec[i]{7..0};
   //j = 1
   //A = crc_pararrel_result[0];
   //loop: 
   //B[i] = A ^ C[i] for i = {7..0};
   //B = ((B[7] << 7) & 0x80) | ((B[6] << 6)&0x40) | ... | ((B[0] << 0)&0x1);
   //A = B ^ crc_pararrel_result[j]
   //j++
   //goto loop
  
   // build C[8]
    //for ( int i = 0; i < CRC_LEN; i++) {
    //    for ( int k = 0; k < CRC_LEN; k++) {
    //        M[i] = (( vec[i][k] << k ) & (1<<k) ) | M[i];
    //    }
    //}

    //for ( int i = 0; i < CRC_LEN; i++) {
    //    printf("M[%d]=%d\n", i, M[i]);
    //}

    for (int j = 0; j < parrallel_block_num; j++) {
        printf("crc_pararrel_result[%d]=0x%x\n", j, crc_pararrel_result[j]);
    }
    b = 0;// b of first block is 0
    for (int j = 0; j < parrallel_block_num; j++) {
        a = crc_pararrel_result[j] ^ b;
        for ( int k = 0; k < CRC_LEN; k++) {
            A[k] = (a>>k) & 0x1;
        }
        printf("0x%x=8'b", a);
        for ( int k = 0; k < CRC_LEN; k++) {
            printf("%d", A[7-k]);
        }
        printf("\n");
        for ( int i = 0; i < CRC_LEN; i++) {
//            B[i] = a ^ M[i];

//            B[i] = ( a * M[i] ) % 2;

//            B[i] = 0;
//            for ( int k = 0; k < CRC_LEN; k++) {
//                B[i] = ( (a&(1<<k)) * vec[i][k] ) + B[i];
//            }
//            B[i] = B[i] % 2;


            B[i] = 0;
            for ( int k = 0; k < CRC_LEN; k++) {
                //B[i] = ( ((a>>k)&0x1) * M[i][k] ) + B[i];
                //B[i] = ( a * M[i][k] ) + B[i];
                B[i] = ( A[k] * M[i][k] ) + B[i];
            }
            B[i] = B[i] % 2;
        }
        b = 0;
        printf("8'b");
        for ( int i = 0; i < CRC_LEN; i++) {
            printf("%d", B[7-i]);
            b = (( B[i] << i ) & (1<<i) ) | b;
        }
        printf("=0x%x\n", b);
    }
    return a;
}
int main() {
    unsigned char data[MAX_DATA_BYTE_LEN] = { 0 };
    unsigned char crc = INIT;
    int byte_length = 256;

//    crc8_vec_init(CRC_PARARREL_BYTE_LEN);

    printf ("verify crc code through https://crccalc.com/\n");
    printf("input data(exit for terminate):");
    scanf("%s", data);
    while ( strcmp(data, "exit") != 0 ) {
        crc = INIT;
        byte_length = strlen(data);
        crc = crc8_calc_standard(crc, data, byte_length);
        printf("data = %s, crc8 = 0x%x\n\n", data, crc);
        
//        crc = INIT;
//        byte_length = strlen(data);
//        crc = crc8_calc_parallel(crc, data, byte_length);
//        printf("data = %s, crc8 = 0x%x\n\n", data, crc);
        
        printf("input data(exit for terminate):");
        scanf("%s", data);
    }

    return 0;
}
