/**
 * extreme_edge_cases
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

size_t cal_size( char** output ){
    if( output == NULL ) return 0;
    size_t size = 0;
    char** cur = output;
    while( *cur != NULL ){
        size++;
        cur++;
    }
    return size;
}

int check(char* output_str[], char** output){
    int i;
    if( cal_size(output_str) != cal_size(output) ) return 0;
    for(i=0; i<(int)cal_size(output_str); i++){
        // printf("output_str[i] = %s, output[i] = %s\n", output_str[i], output[i]);
        // printf("size = %d, size = %d\n", sizeof(output_str[i]), sizeof(output[i]));
        if( strcmp(output_str[i], output[i])!=0 ){
            return 0;
    }
    }
    return 1;
}

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Implement me!
    char** output_str;
    // Testcase1
    output_str = camelCaser("        as     if.  jkfodljlsn.AAAA");
    char* output1[]={"asIf",
        "jkfodljlsn",
        NULL};
    if( !check(output_str, output1) ) return 0;
    else printf("sucess test 1\n");
    destroy(output_str);

    //Testcase2
    output_str = camelCaser("");
    // for(int i=0; i<(int)cal_size(output_str); i++){
    //     printf("output_str[i] = %s\n", output_str[i]);
    // }
    char* output2[]={NULL};
    if( !check(output_str, output2)) return 0;
    else printf("sucess test 2\n");
    destroy(output_str);

    //Testcase3
    output_str = camelCaser(" A2c 3e5 123   iS a WORD? No! disapeear ");
    char* output3[]={"a2c3E5123IsAWord", "no", NULL};
    if( !check(output_str, output3)) return 0;
    else printf("sucess test 3\n");
    destroy(output_str);
    
    //Testcase4
    output_str = camelCaser(" \n \0 . b");
    char* output4[]={NULL};
    if( !check(output_str, output4)) return 0;
    else printf("sucess test 4\n");
    destroy(output_str);

    //Testcase5
    output_str = camelCaser(" \n &A2c 3e5 123   iS a WORD? No! = @ disapeear\t ");
    char* output5[]={"", "a2c3E5123IsAWord", "no", "", "", NULL};
    if( !check(output_str, output5)) return 0;
    else printf("sucess test 5\n");
    destroy(output_str);

    //Testcase6
    output_str = camelCaser("@.!");
    char* output6[]={"", "", "", NULL};
    if( !check(output_str, output6)) return 0;
    else printf("sucess test 6\n");
    destroy(output_str);

    //Testcase7
    output_str = camelCaser(" \a   \b \n     \t     ");
    char* output7[]={NULL};
    if( !check(output_str, output7)) return 0;
    else printf("sucess test 7\n");
    destroy(output_str);

    //Testcase8
    output_str = camelCaser("123@.12d!");
    char* output8[]={"123", "", "12d", NULL};
    if( !check(output_str, output8)) return 0;
    else printf("sucess test 8\n");
    destroy(output_str);

    //Testcase9
    output_str = camelCaser("@1.2!");
    char* output9[]={"", "1", "2", NULL};
    if( !check(output_str, output9)) return 0;
    else printf("sucess test 9\n");
    destroy(output_str);

    //Testcase10
    output_str = camelCaser("@");
    char* output10[]={"", NULL};
    if( !check(output_str, output10)) return 0;
    else printf("sucess test 10\n");
    destroy(output_str);

    //Testcase11
    output_str = camelCaser("\a\bas\a\bas, f62 and \a y,");
    char* output11[]={"\a\bas\a\bas", "f62And\aY", NULL};
    if( !check(output_str, output11)) return 0;
    else printf("sucess test 11\n");
    destroy(output_str);

    //Testcase12
    output_str = camelCaser(" qwr \v \af, food  ");
    char* output12[]={"qwr\aF", NULL};
    if( !check(output_str, output12)) return 0;
    else printf("sucess test 12\n");
    destroy(output_str);

    //Testcase13
    output_str = camelCaser("76\v9 IQ\\T : M\bE q78pE\nm Gu\ni?");
    char* output13[]={"769Iq", "t", "m\beQ78peMGuI", NULL};
    if( !check(output_str, output13)) return 0;
    else printf("sucess test 13\n");
    destroy(output_str);

    //Testcase14
    output_str = camelCaser("hello\n\tthere, how are u doin' today\" my lil' homie?");
    char* output14[]={"helloThere", "howAreUDoin", "today", "myLil", "homie", NULL};
    if( !check(output_str, output14)) return 0;
    else printf("sucess test 14\n");
    destroy(output_str);

    //Testcase15
    output_str = camelCaser(" \0None of these should appear.");
    char* output15[]={NULL};
    if( !check(output_str, output15)) return 0;
    else printf("sucess test 15\n");
    destroy(output_str);
    return 1;
}