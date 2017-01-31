#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lily_api_embed.h"
#include "lily_api_value.h"
#include "lily_api_msgbuf.h"

/**
package xstr

Provides a few extra operations on strings.
*/

// this is private to be core, so let's define it again ;)
static const char move_table[256] =
{
     /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 1 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 2 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 4 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 5 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 6 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 7 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 8 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* A */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* B */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* C */ 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* D */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* E */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
/* F */ 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/**
define next(s: String, index: Integer): Tuple[String,Integer]

Grab the following character at index. Returns a tuple containing
the character at the index, and the index of the next character.

The returned index is -1 for the last character; thereafter ["",-1]
is returned.
*/
void lily_xstr__next(lily_state *s)
{
    const char *input = lily_arg_string_raw(s,0);
    int index  = lily_arg_integer(s,1);

    // how many chars in this character?
    int to_copy = index > -1 ? move_table[(uint8_t)input[index]] : 0;

    // make a copy!
    lily_string_val *string_result = lily_new_string_sized(input + index, to_copy);

    // and let's move onto the next....
    index += to_copy;
    if (index > -1 && input[index] == 0) { // ALWAYS nul-terminated...
        index = -1;
    }

    // pack the character and the next index into a tuple
    lily_container_val *lv = lily_new_list(2);
    lily_nth_set(lv,0,lily_box_string(s,string_result));
    lily_nth_set(lv,1,lily_box_integer(s,index));
    lily_return_tuple(s,lv);
}

/**
define unsafe_str_index (s: String, index: Integer): String

Lily's `String` class wrongly allowed subscripting a while back. This simulates
it, until the libraries herein can be weaned off of it by using `ByteString` and
`Byte` iteration.
*/
void lily_xstr__unsafe_str_index(lily_state *s)
{
    char *input = lily_arg_string_raw(s, 0);
    int index = lily_arg_integer(s, 1);
    char *ch;

    if (index >= 0) {
        ch = &input[0];
        while (index && move_table[(unsigned char)*ch] != 0) {
            ch += move_table[(unsigned char)*ch];
            index--;
        }
        if (move_table[(unsigned char)*ch] == 0)
            lily_IndexError(s, "Index %d is out of range.\n", index);
    }
    else {
        char *stop = &input[0];
        int len = lily_string_length(lily_arg_string(s, 0));
        ch = &input[len];
        while (stop != ch && index != 0) {
            ch--;
            if (move_table[(unsigned char)*ch] != 0)
                index++;
        }
        if (index != 0)
            lily_IndexError(s, "Index %d is out of range.\n", index);
    }

    int count = move_table[(unsigned char)*ch];
    lily_return_string(s, lily_new_string_sized(ch, count));
}

/**
define parse_i (s: String, base: *Integer=10): Option[Integer]

Like String.parse_i, except that you can provide a base (default 10)
*/
void lily_xstr__parse_i (lily_state *s)
{
    const char* s_ = lily_arg_string_raw(s,0);
    int64_t base = (lily_arg_count(s) > 1) ? lily_arg_integer(s,1) : 10;
    char *endp;
    int64_t return_some = strtoll(s_,&endp, base);

    if (*s_ == '\0' ||  *endp != '\0') {
        lily_return_none(s);
        return;
    }
    
    lily_container_val *some_val = lily_new_some();
    lily_nth_set(some_val, 0, lily_box_integer(s, return_some));
    lily_return_variant(s,some_val);
}

/**
define parse_d (s: String): Option[Double]

Convert a string to a Double, returning None if unsuccesful
*/
void lily_xstr__parse_d (lily_state *s)
{
    const char* s_ = lily_arg_string_raw(s,0);
    char *endp;
    double return_some = strtod(s_,&endp);

    if (*s_ == '\0' ||  *endp != '\0') {
        lily_return_none(s);
        return;
    }
    
    lily_container_val *some_val = lily_new_some();
    lily_nth_set(some_val, 0, lily_box_double(s, return_some));
    lily_return_variant(s,some_val);
}

/**
define shell (cmd: String): String

Read all the output of a shell command using popen
*/
void lily_xstr__shell(lily_state *s)
{
    const char *cmd = lily_arg_string_raw(s,0);
    lily_msgbuf *msgbuf = lily_new_msgbuf(256);
    FILE *f = popen(cmd,"r");
    char buffer[128];
    int count;

    do {
        count = fread(buffer, 1, sizeof(buffer), f);
        buffer[count] = '\0';
        lily_mb_add(msgbuf, buffer);
    } while (count == sizeof(buffer));

    lily_string_val *sv = lily_new_string(lily_mb_get(msgbuf));
    lily_free_msgbuf(msgbuf);
    pclose(f);
    lily_return_string(s, sv);
}

static void concat(lily_state *s, lily_msgbuf *msgbuf, char ch, lily_container_val *lv)
{
   int i;
   lily_mb_flush(msgbuf);
   for (i = 0; i < lily_container_num_values(lv); i++) {
      lily_value *v = lily_nth_get(lv,i);
      if (i > 0)
         lily_mb_add_char(msgbuf,ch);
      lily_mb_add_value(msgbuf, s, v);
   }
}

/**
define print (values:1...)

Print several values
*/
void lily_xstr__print(lily_state *s)
{
    lily_container_val *lv = lily_arg_container(s,0);

    lily_msgbuf *msgbuf = lily_get_clean_msgbuf(s);
    concat(s,msgbuf,'\t',lv);
    lily_mb_add_char(msgbuf,'\n');
    fputs(lily_mb_get(msgbuf),stdout);
    lily_mb_flush(msgbuf);
    lily_return_unit(s);
}

/**
define concat(delim: String, values:1...):String

Concatenate some arbitrary values using the single-character delimiter
*/
void lily_xstr__concat(lily_state *s)
{
    const char *delim = lily_arg_string_raw(s,0);
    lily_container_val *lv = lily_arg_container(s,1);

    lily_msgbuf *msgbuf = lily_get_clean_msgbuf(s);
    concat(s,msgbuf,delim[0],lv);
    lily_return_string(s,lily_new_string(lily_mb_get(msgbuf)));
    lily_mb_flush(msgbuf);
}

static int char_index(const char *s, int idx, char ch)
{
    const char *P = strchr(s+idx,ch);
    if (! P)
        return -1;
    else
        return (int)((uintptr_t)P - (uintptr_t)s);
}

// note the assumption ;)
static int convert_i(char d)
{
    return (int)d - (int)'0';
}

/**
define exit(code: Integer)

Exit the program with the given return code
*/
void lily_xstr__exit(lily_state *s)
{
    int64_t code = lily_arg_integer(s,0);
    exit((int)code);
    
}

#include "dyna_xstr.h"
