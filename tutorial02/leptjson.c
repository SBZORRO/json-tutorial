#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <math.h>
#include <errno.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

#define ISDIGIT(ch)       ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)   ((ch) >= '1' && (ch) <= '9')

#define ISWHITESPACE(ch)  ((ch) == ' ' || (ch) =='\t' || (ch) == '\n' || (ch) == '\r')

typedef struct {
  const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
  const char *p = c->json;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    p++;
  c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* l, lept_type type) {
  EXPECT(c, l[0]);
  size_t i;
  for (i = 0; l[i+1]; i++) {
    if (c->json[i] != l[i+1]) {
      return LEPT_PARSE_INVALID_VALUE;
    }
  }
  c->json +=i;
  v->type = type;
  return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
  char* end;
  /* \TODO validate number */
  int res = validate_number(c,v);
  if (res != LEPT_PARSE_OK){
    return res;
  }
  errno = 0;
  v->n = strtod(c->json, &end);
  if(errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL)){
    return LEPT_PARSE_NUMBER_TOO_BIG;
  }
  if (c->json == end)
    return LEPT_PARSE_INVALID_VALUE;
  c->json = end;
  v->type = LEPT_NUMBER;
  return LEPT_PARSE_OK;
}

int validate_number(lept_context* c, lept_value* v) {
  char* head = c->json;
  char cur = head[0];
  char next = head[1];

  if (cur == '-' ){
    head++;
    cur = head[0];
    next = head[1];
  } else if (!ISDIGIT(cur)){
    return LEPT_PARSE_INVALID_VALUE;
  }

  if (cur == '0' ){

    if (next == '\0') {
      return LEPT_PARSE_OK;
    }    else if (next != '.'){

      return LEPT_PARSE_ROOT_NOT_SINGULAR;
    } else {
      head = head + 2;
      cur = *head;
    }
  }

  while(ISDIGIT(cur)){
    head++;
    cur = *head;
  }

  if (cur == '\0') {
    return LEPT_PARSE_OK;
  }

  if (cur == '.' ) {
    head++;
    cur = *head;

    if (cur == '\0') {
      return LEPT_PARSE_INVALID_VALUE;
    }

    while(ISDIGIT(cur)){
      head++;
      cur = *head;
    }
  }

  if ( cur == 'e' || cur == 'E'){
    head++;
    cur = *head;

    if (cur == '+' || cur == '-') {
      head++;
      cur = *head;
    }

    if (!ISDIGIT1TO9(cur)) {
      return LEPT_PARSE_INVALID_VALUE;
    }
    while(ISDIGIT(cur)){
      head++;
      cur = *head;
    }
  }

  if (cur != '\0'){
    return LEPT_PARSE_ROOT_NOT_SINGULAR;
  }

  return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
  switch (*c->json) {
  case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
  case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
  case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
  default:   return lept_parse_number(c, v);
  case '\0': return LEPT_PARSE_EXPECT_VALUE;
  }
}

int lept_parse(lept_value* v, const char* json) {
  lept_context c;
  int ret;
  assert(v != NULL);
  c.json = json;
  v->type = LEPT_NULL;
  lept_parse_whitespace(&c);
  if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
    lept_parse_whitespace(&c);
    if (*c.json != '\0') {
      v->type = LEPT_NULL;
      ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
    }
  }
  return ret;
}

lept_type lept_get_type(const lept_value* v) {
  assert(v != NULL);
  return v->type;
}

double lept_get_number(const lept_value* v) {
  assert(v != NULL && v->type == LEPT_NUMBER);
  return v->n;
}
