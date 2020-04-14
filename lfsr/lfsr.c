#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint8_t  unsigned char
#define int32_t  int

#define DISTANCE_MAX 10 //2^0.2^1.2^2.2^3.2^4.2^5.2^6.2^7.2^8.2^9.MAX
#define POLY_LEN_MAX 32 // 

static uint32_t lfsr_len = 0;
static uint32_t lfsr_rs_feed = 0;
static uint32_t lfsr_rs_feed_msb = 0;
static uint32_t lfsr_out_mask = 0;
static uint32_t lfsr_max = 0;

// poly_str to {poly_len, poly_val}: poly_val w/o poly_len
uint32_t poly_str2val( uint8_t *poly_str, uint32_t *poly_val)
{
    uint32_t poly_len = 0;
    uint32_t poly_data = 0;

    uint8_t *poly_sub_str = poly_str;
    uint8_t *tmp_poly_sub_str = poly_str;
    int32_t poly_iter_index = POLY_LEN_MAX; // start from MSB
    uint8_t poly_iter_str[3] = { 0 };
    uint8_t poly_sub_str_len = strlen(poly_sub_str);

    while ( ( poly_sub_str_len != 0 ) && ( poly_iter_index >= 0 ) ) {
        if ( 0 == poly_iter_index ) {
            sprintf(poly_iter_str, "%d", 1); 
        } else if ( 1 == poly_iter_index ) {
            sprintf(poly_iter_str, "%s", "x"); 
        } else {
            sprintf(poly_iter_str, "x%d", poly_iter_index); 
        }

        if ( ( tmp_poly_sub_str = strstr( poly_sub_str, poly_iter_str ) ) != NULL ) {
            poly_sub_str = tmp_poly_sub_str+1;//skip current char(x)
            if ( 0 == poly_len ) {
                poly_len = poly_iter_index;
            } else { // poly_data w/o poly_len
                poly_data = poly_data | ( ( 1 << poly_iter_index ) );
            }
        }
        poly_iter_index--;
        poly_sub_str_len = strlen( poly_sub_str );
    }

    *poly_val = poly_data;
    return poly_len;
}

uint32_t lfsr_init( uint8_t* poly_str ) {
    uint32_t poly_val = 0;
    uint32_t poly_len = poly_str2val(poly_str, &poly_val);

    lfsr_len = poly_len;
    lfsr_rs_feed = ( 1 << ( poly_len - 1 ) ) | ( poly_val >> 1 );
    lfsr_rs_feed_msb =( 1 << ( poly_len - 1 ) );// 2^(n-1)
    lfsr_out_mask = ( 32 == poly_len ) ? 0xFFFFFFFF : ( ( 1 <<  poly_len ) - 1 ) ; // 2^n - 1
    lfsr_max = ( 32 == poly_len ) ? 0xFFFFFFFF : ( ( 1 <<  poly_len ) - 1 ) ; // 2^n - 1
    printf("POLY_STR:%s\n=======>\nPOLY_LEN:%d, POLY_VAL:0x%x\n=======>\nLFSR_RS_FEED:0x%x, LFSR_RS_FEED_MSB:0x%x, LFSR_OUT_MASK:0x%x\n", poly_str, 
            poly_len, poly_val, lfsr_rs_feed, lfsr_rs_feed_msb, lfsr_out_mask);

    return 0;

}

// right shift, pop lsb to msb after xor with 1'b0
uint32_t lfsr_galoris_rs(uint32_t lfsr_in, uint32_t msb_xor)
{
    uint32_t lfsr_out = 0;
    uint32_t lsb = 0;

    lsb = lfsr_in & 0x1;
    lfsr_out = lfsr_in >> 1;
    if ( lsb ) {
        lfsr_out = ( lfsr_out ^ lfsr_rs_feed );
    }
    lfsr_out = msb_xor ? lfsr_out ^ lfsr_rs_feed_msb : lfsr_out; // reverse msb each time

    return lfsr_out & lfsr_out_mask;
}

uint32_t lfsr_simulation(uint32_t mode, uint32_t pkt_total_num, uint32_t pkt_log_ratio, uint32_t result_vec[])
{
    uint32_t pkt_id = 0;
    uint32_t pkt_id_last = 0;

    uint32_t distance = 0;
    uint32_t distance_range = 0;
    uint32_t distance_range_vec[DISTANCE_MAX+1] = { 0 };

    uint32_t pkt_log_thrd = lfsr_out_mask / pkt_log_ratio;
    uint32_t pkt_log_num = 0;

    uint32_t lfsr_out = (uint32_t)random();

    while ( pkt_id < pkt_total_num ) {
        lfsr_out = lfsr_galoris_rs(lfsr_out, mode);
        if ( lfsr_out > pkt_log_thrd ) { // not captured
            pkt_id++;
            continue;
        }
        if (  pkt_id != 0 ) { // captured
            distance = pkt_id - pkt_id_last;

            distance_range = 0;
            for ( distance_range = 0; distance_range < DISTANCE_MAX; distance_range++ ) {
                if ( distance <= ( 1 << distance_range ) ) {
                    distance_range_vec[distance_range]++;
                    break;
                } else if ( DISTANCE_MAX == ( distance_range + 1 ) ) { // last distance
                    distance_range_vec[distance_range + 1]++;
                    break;
                }
            }

        }
        pkt_id_last = pkt_id;
        pkt_id++;
        pkt_log_num++;
    }
    
    printf("\nLogCfg=1/%-10d, LFRS_MODE:%d\n", pkt_log_ratio, mode);
    printf("LogResult=1/%-10.1f, pkt_total_num=%d, pkt_log_num=%d\n", ( pkt_log_num != 0 ) ? (float)pkt_total_num/pkt_log_num : 0, pkt_total_num, pkt_log_num);
    for ( distance_range = 0; distance_range <= DISTANCE_MAX; distance_range++ ) {
 //       if ( DISTANCE_MAX == distance_range ) {
 //           printf("pkt_distance  > %-10d: %-10d\n", ( 1 << (DISTANCE_MAX - 1) ) , distance_range_vec[distance_range]);
 //       } else {
 //           printf("pkt_distance <= %-10d: %-10d\n", ( 1 << distance_range ) , distance_range_vec[distance_range]);
 //       }
        result_vec[distance_range] = distance_range_vec[distance_range];
    }

    return 0;

}

int main(int argc, char* argv[])
{
    uint8_t *poly_str = "x25+x24+x23+x22+x21+x20+x19+x18+x17+x16+x15+x14+x13+x12+x11+x10+x9+x8+x7+x6+x5+x4+1"; 
    uint32_t simulation_round_max = 2;
    uint32_t pkt_total_num = 0;
    uint32_t pkt_log_exponent = 0;
    uint32_t pkt_log_ratio = 0;

    uint32_t distance_range = 0;
    uint32_t pkt_log_num[2] = { 0 };

    uint32_t result_vec[2][DISTANCE_MAX+1] = { { 0 } };

    lfsr_init( ( argv[1] != NULL ) ? (uint8_t*)argv[1] : poly_str );

    pkt_total_num = lfsr_max * simulation_round_max;
    for ( pkt_log_exponent = 0; pkt_log_exponent <= lfsr_len; pkt_log_exponent++ ) {
        pkt_log_ratio = ( 1 << pkt_log_exponent );

        pkt_log_num[0] = 0;
        pkt_log_num[1] = 0;
        lfsr_simulation(0, pkt_total_num, pkt_log_ratio, result_vec[0]);
        lfsr_simulation(1, pkt_total_num, pkt_log_ratio, result_vec[1]);

        printf("\nLogCfg=1/%d, LFRS_MODE:[%d]/[%d]\n", pkt_log_ratio, 0, 1);
        for ( distance_range = 0; distance_range <= DISTANCE_MAX; distance_range++ ) {
            pkt_log_num[0] += result_vec[0][distance_range];
            pkt_log_num[1] += result_vec[1][distance_range];
            if ( DISTANCE_MAX == distance_range ) {
                printf("pkt_distance  > %d: [%d]/[%d]\n", ( 1 << (DISTANCE_MAX - 1) ) , result_vec[0][distance_range], result_vec[1][distance_range]);
                printf("pkt_log_total : [%d]/[%d]\n", pkt_log_num[0], pkt_log_num[1]);
            } else {
                printf("pkt_distance <= %d: [%d]/[%d]\n", ( 1 << distance_range ) , result_vec[0][distance_range], result_vec[1][distance_range]);
            }
        }


    }

    return 0;
} 
