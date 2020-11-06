struct {
  u32 len;
  u32* ptr;
} typedef t_array_i;

struct {
  u32 len;
  r32* ptr;
} typedef t_array_f;

struct {
  u32 len;
  t_string* ptr;
} typedef t_array_s;

struct {
  bool error;
  t_array_i colorCycle;
  u32 backgroundColor;
  i64 useInterpolation;
} typedef t_app_config;

global t_app_config app_config;

internal void config_load_default(t_app_config* appConfig) {
  appConfig->error = false;
  appConfig->colorCycle.len = 2;
  appConfig->colorCycle.ptr = malloc(appConfig->colorCycle.len * sizeof(u32));
  appConfig->colorCycle.ptr[0] = 0xff000000;
  appConfig->colorCycle.ptr[1] = 0xffffffff;
  appConfig->backgroundColor = 0;
  appConfig->useInterpolation = false;
}

enum {
  TOKEN_TYPE_UNDEFINED,
  
  TOKEN_TYPE_IDENTIFIER,
  TOKEN_TYPE_INTEGER,
  TOKEN_TYPE_FLOAT,
  TOKEN_TYPE_STRING,
  
  TOKEN_TYPE_ARRAY_OPEN,
  TOKEN_TYPE_ARRAY_CLOSE,
  TOKEN_TYPE_ASSIGMENT,
  
  TOKEN_TYPE_EOF,
} typedef t_token_type;

struct {
  t_token_type type;
  char* start;
  u32 len;
} typedef t_token;

struct {
  t_token* v;
  u32 count;
  u32 alloc;
} typedef t_token_list;

internal void token_push(t_token_list* list, t_token token) {
  if(list->count + 1 > list->alloc) {
    list->alloc *= 2;
    if(list->alloc == 0) list->alloc = 1;
    list->v = realloc(list->v, list->alloc * sizeof(t_token));
  }
  list->v[list->count] = token;
  list->count += 1;
}

internal inline bool in_range(char c, char s, char e) {return(c>=s && c<=e);}

internal inline bool is_letter(char c) { return(in_range(c,'a','z') || in_range(c, 'A', 'Z') || (c=='_'));}
internal inline bool is_dec_digit(char c) { return(in_range(c, '0', '9')); }
internal inline bool is_hex_digit(char c) { return(in_range(c, '0', '9') || in_range(c, 'a', 'f') || in_range(c,'A','F')); }
internal inline bool is_bin_digit(char c) { return(in_range(c, '0', '9') || in_range(c, 'a', 'f') || in_range(c,'A','F')); }
internal inline bool is_whitespace(char c) { return(c==' ' || c==',' || c==';' || in_range(c, 1, 31)); }
internal inline bool is_alphanumeric(char c) { return(is_letter(c) || is_dec_digit(c)); }

internal inline int get_dec_digit(char c) { return((int)(c - '0')); }
internal inline int get_hex_digit(char c) { 
  if(in_range(c, 'a', 'f')) return(10 + (int)(c - 'a'));
  if(in_range(c, 'A', 'F')) return(10 + (int)(c - 'A'));
  if(in_range(c, '0', '9')) return((int)(c - '0'));
  return(-1);
}

#define advance_to(state)   {index+=1;token.len+=1; goto state;}
#define token_start()       token.start = text + index;token.len = 0
#define token_finish(state) {token_push(&tokens, token); goto state;}
#define state_start(t)      token.type = (t); char c = text[index]
internal t_token_list lex_config_file(t_file_data fileData) {
  t_token_list tokens = {0};
  {
    t_token token = {0};
    char* text = (char*)fileData.ptr;
    u32 index = 0;
    
    state_main: {
      if(index == fileData.size) goto end;
      state_start(0);
      token_start();
      if(is_letter(c))          {advance_to(state_identifier)}
      else if(c=='0')           {advance_to(state_integer_prefix)}
      else if(c=='.')           {advance_to(state_float)}
      else if(c=='{')           {advance_to(state_array_start)}
      else if(c=='}')           {advance_to(state_array_end)}
      else if(c=='"')           {advance_to(state_string)}
      else if(c=='=')           {advance_to(state_assigment)}
      else if(is_dec_digit(c))  {advance_to(state_dec_integer)}
      else if(is_whitespace(c)) {advance_to(state_main)}
      else if(c==0)             goto end;
      else                      goto error;
    }
    
    state_identifier: {
      state_start(TOKEN_TYPE_IDENTIFIER);
      if(is_alphanumeric(c))    {advance_to(state_identifier)}
      else if(is_whitespace(c)) {token_finish(state_main)}
      else if(c=='}')           {token_finish(state_main)}
      else if(c=='=')           {token_finish(state_main)}
      else if(c==0)             {token_finish(state_main)}
      else                      goto error;
    }
    
    state_integer_prefix: {
      state_start(TOKEN_TYPE_INTEGER);
      if(is_dec_digit(c))       {advance_to(state_dec_integer)}
      else if(c=='x')           {advance_to(state_hex_integer)}
      else if(c=='b')           {advance_to(state_bin_integer)}
      else if(c=='.')           {advance_to(state_float)}
      else if(c=='=')           {token_finish(state_main)}
      else if(c=='}')           {token_finish(state_main)}
      else if(is_whitespace(c)) {token_finish(state_main)}
      else if(c==0)             goto error;
      else                      goto error;
    }
    
    state_dec_integer: {
      state_start(TOKEN_TYPE_INTEGER);
      if(is_dec_digit(c))       {advance_to(state_dec_integer)}
      else if(c=='.')           {advance_to(state_float)}
      else if(c=='}')           {token_finish(state_main)}
      else if(c=='=')           {token_finish(state_main)}
      else if(c==0)             {token_finish(state_main)}
      else if(is_whitespace(c)) {token_finish(state_main)}
      else                      goto error;
    }
    
    state_hex_integer: {
      state_start(TOKEN_TYPE_INTEGER);
      if(is_hex_digit(c))       {advance_to(state_hex_integer)}
      else if(c=='}')           {token_finish(state_main)}
      else if(c=='=')           {token_finish(state_main)}
      else if(c==0)             {token_finish(state_main)}
      else if(is_whitespace(c)) {token_finish(state_main)}
      else                      goto error;
    }
    
    state_bin_integer: {
      state_start(TOKEN_TYPE_INTEGER);
      if(is_bin_digit(c))       {advance_to(state_bin_integer)}
      else if(c=='}')           {token_finish(state_main)}
      else if(c=='=')           {token_finish(state_main)}
      else if(c==0)             {token_finish(state_main)}
      else if(is_whitespace(c)) {token_finish(state_main)}
      else                      goto error;
    }
    
    state_float: {
      state_start(TOKEN_TYPE_FLOAT);
      if(is_dec_digit(c))       {advance_to(state_float)}
      else if(c=='}')           {token_finish(state_main)}
      else if(c=='=')           {token_finish(state_main)}
      else if(c==0)             {token_finish(state_main)}
      else if(is_whitespace(c)) {token_finish(state_main)}
      else                      goto error;
    }
    
    state_array_start: {
      state_start(TOKEN_TYPE_ARRAY_OPEN);
      debug_variable_unused(c);
      {token_finish(state_main)}
    }
    
    state_array_end: {
      state_start(TOKEN_TYPE_ARRAY_CLOSE);
      debug_variable_unused(c);
      {token_finish(state_main)}
    }
    
    state_assigment: {
      state_start(TOKEN_TYPE_ASSIGMENT);
      debug_variable_unused(c);
      {token_finish(state_main)}
    }
    
    state_string: {
      state_start(TOKEN_TYPE_STRING);
      if(c=='\\')               {advance_to(state_string_screen)}
      else if(c=='"')           {advance_to(state_string_end)}
      else if(c==0)             goto error;
      else                      {advance_to(state_string)}
    }
    
    state_string_screen: {advance_to(state_string)}
    state_string_end:    {token_finish(state_main)}
  }
  
  end:
  return(tokens);
  error: {
    if(tokens.v) free(tokens.v);
    tokens.count = 0;
    tokens.alloc = 0;
    return(tokens);
  }
}


enum {
  TYPE_INTEGER,
  TYPE_STRING,
  TYPE_FLOAT,
  TYPE_ARRAY_INTEGER,
  TYPE_ARRAY_STRING,
  TYPE_ARRAY_FLOAT,
} typedef t_symbol_type;

struct {
  t_string name;
  t_symbol_type type;
  union {
    i64 value_i;
    r64 value_f;
    t_string value_s;
    t_array_i value_ai;
    t_array_f value_af;
    t_array_s value_as;
  };
} typedef t_symbol;

struct {
  t_symbol* v;
  u32 count;
  u32 alloc;
} typedef t_symbol_table;

internal t_symbol* symbol_push(t_symbol_table* list, t_symbol symbol) {
  if(list->count + 1 > list->alloc) {
    list->alloc *= 2;
    if(list->alloc == 0) list->alloc = 1;
    list->v = realloc(list->v, list->alloc * sizeof(t_symbol));
  }
  t_symbol* result = list->v + list->count;
  *result = symbol;
  list->count += 1;
  return(result);
}

internal t_symbol* symbol_find_by_string(t_symbol_table* symbols, t_string name) {
  for(u32 symbolIndex = 0; symbolIndex < symbols->count; symbolIndex += 1) {
    t_symbol* symbol = symbols->v + symbolIndex;
    if(string_compare(symbol->name, name)) {
      return(symbol);
    }
  }
  return(0);
}

internal t_symbol* symbol_find_by_token(t_symbol_table* symbols, t_token* identifier) {
  t_string identifierName;
  identifierName.ptr = identifier->start;
  identifierName.len = identifier->len;
  t_symbol* result = symbol_find_by_string(symbols, identifierName);
  return(result);
}

internal t_symbol* symbol_create_from_token(t_symbol_table* symbols, t_token* identifier) {
  t_string identifierName;
  identifierName.ptr = identifier->start;
  identifierName.len = identifier->len;
  t_symbol newSymbol = {0};
  newSymbol.name = string_copy_mem(identifierName);
  t_symbol* result = symbol_push(symbols, newSymbol);
  return(result);
}

internal t_symbol* symbol_find_or_create_from_token(t_symbol_table* symbols, t_token* identifier) {
  t_string identifierName;
  identifierName.ptr = identifier->start;
  identifierName.len = identifier->len;
  t_symbol* result = symbol_find_by_string(symbols, identifierName);
  if(!result) {
    t_symbol newSymbol = {0};
    newSymbol.name = string_copy_mem(identifierName);
    result = symbol_push(symbols, newSymbol);
  }
  return(result);
}

internal void symbol_copy_value(t_symbol* dest, t_symbol* source) {
  dest->type = source->type;
  if(dest->type == TYPE_INTEGER) {
    dest->value_i = source->value_i;
  }
  else if(dest->type == TYPE_FLOAT) {
    dest->value_f = source->value_f;
  }
  else if(dest->type == TYPE_STRING) {
    dest->value_s = string_copy_mem(source->value_s);
  }
  else if(dest->type == TYPE_ARRAY_INTEGER) {
    dest->value_ai.len = source->value_ai.len;
    dest->value_ai.ptr = malloc(dest->value_ai.len * sizeof(u32));
    for(u32 index = 0; index < dest->value_ai.len; index += 1) {
      dest->value_ai.ptr[index] = source->value_ai.ptr[index];
    }
  }
  else if(dest->type == TYPE_ARRAY_FLOAT) {
    dest->value_af.len = source->value_af.len;
    dest->value_af.ptr = malloc(dest->value_af.len * sizeof(r32));
    for(u32 index = 0; index < dest->value_af.len; index += 1) {
      dest->value_af.ptr[index] = source->value_af.ptr[index];
    }
  }
  else if(dest->type == TYPE_ARRAY_STRING) {
    dest->value_as.len = source->value_as.len;
    dest->value_as.ptr = malloc(dest->value_as.len * sizeof(t_string));
    for(u32 index = 0; index < dest->value_as.len; index += 1) {
      dest->value_as.ptr[index] = string_copy_mem(source->value_as.ptr[index]);
    }
  }
  else assert(0);
}

// TODO(bumbread): negative integers
internal i64 token_parse_integer(t_token* token) {
  char* c = token->start;
  u32 index = 0;
  
  u32 base = 10;
  if(token->len > 1) {
    if(c[1] == 'x') {base = 16; c+= 2; index += 2;}
    else if(c[1] == 'b') {base = 2; c += 2; index += 2;}
  }
  
  i64 result = 0;
  while(index < token->len) {
    int digit = get_hex_digit(*c);
    result *= base;
    result += (u32)digit;
    index += 1;
    c += 1;
  }
  
  return(result);
}

// TODO(bumbread): negative floating point numbers
internal r64 token_parse_float(t_token* token) {
  char* c = token->start;
  u32 index = 0;
  
  r64 result = 0;
  while(index < token->len) {
    if(*c == '.') break;
    int digit = get_hex_digit(*c);
    result *= 10.0;
    result += (r64)digit;
    index += 1;
    c += 1;
  }
  if(*c == '.') {
    c += 1;
    index += 1;
  }
  r64 divisor = 0;
  while(index < token->len) {
    if(*c == '.') break;
    int digit = get_hex_digit(*c);
    result *= 10.0;
    divisor /= 10.0;
    result += (r64)digit;
    index += 1;
    c += 1;
  }
  
  return(result*divisor);
}


internal t_string token_parse_string(t_token* token) {
  char* c = token->start;
  u32 index = 0;
  
  assert(token->len >= 2);
  t_string string;
  string.len = token->len - 2;
  string.ptr = malloc((string.len - 1) * sizeof(char));
  string.ptr[string.len] = 0;
  
  index = 1;
  c += 1;
  u32 i = 0;
  while(index < token->len) {
    if(*c == '\\') {
      c += 1;
      index += 1;
      switch(*c) {
        case('n'): string.ptr[i] = '\n'; break;
        case('t'): string.ptr[i] = '\t'; break;
      }
      c += 1;
      index += 1;
    }
    else {
      string.ptr[i] = *c;
      c += 1;
      index += 1;
    }
    i += 1;
  }
  
  return(string);
}

internal bool write_token_value_to_symbol(t_symbol_table* symbols, t_symbol* target, t_token* value) {
  if(value->type == TOKEN_TYPE_INTEGER) {
    target->type = TYPE_INTEGER;
    target->value_i = token_parse_integer(value);
  }
  else if(value->type == TOKEN_TYPE_FLOAT) {
    target->type = TYPE_FLOAT;
    target->value_f = token_parse_float(value);
  }
  else if(value->type == TOKEN_TYPE_STRING) {
    target->type = TYPE_STRING;
    target->value_s = string_copy_mem(token_parse_string(value));
  }
  else if(value->type == TOKEN_TYPE_IDENTIFIER) {
    t_symbol* source = symbol_find_by_token(symbols, value);
    if(source == 0) return(false); // TODO(bumbread): do we want this to be less strict?
    symbol_copy_value(target, source);
  }
  else assert(0);
  return(true);
}

internal t_token_type get_token_source_type(t_symbol_table* symbols, t_token* token) {
  switch((u32)token->type) {
    case(TOKEN_TYPE_INTEGER): return(token->type);
    case(TOKEN_TYPE_FLOAT): return(token->type);
    case(TOKEN_TYPE_STRING): return(token->type);
    case(TOKEN_TYPE_IDENTIFIER): {
      t_symbol* source = symbol_find_by_token(symbols, token);
      if(source == 0) return(TOKEN_TYPE_UNDEFINED);
      switch((u32)source->type) {
        case(TYPE_INTEGER): return(TOKEN_TYPE_INTEGER);
        case(TYPE_FLOAT): return(TOKEN_TYPE_FLOAT);
        case(TYPE_STRING): return(TOKEN_TYPE_STRING);
        default: return(TOKEN_TYPE_UNDEFINED);
      }
    }
    default: assert(0);
  }
  return(TOKEN_TYPE_UNDEFINED);
}

internal t_symbol_type get_corresponding_array_type(t_token_type elementType) {
  t_symbol_type arrayType = 0;
  switch((u32)elementType) {
    case(TOKEN_TYPE_INTEGER): arrayType = TYPE_ARRAY_INTEGER; break;
    case(TOKEN_TYPE_FLOAT): arrayType = TYPE_ARRAY_FLOAT; break;
    case(TOKEN_TYPE_STRING): arrayType = TYPE_ARRAY_STRING; break;
    default: assert(0);
  }
  return(arrayType);
}

internal bool write_token_array_to_symbol(t_symbol_table* symbols, t_symbol* target,
                                          t_token* array, u32 arrayCount) {
  if(arrayCount == 0) {
    target->type = TYPE_ARRAY_INTEGER;
    target->value_ai.len = 0;
    target->value_ai.ptr = 0;
    return(true);
  }
  
  t_token_type elementType = get_token_source_type(symbols, &array[0]);
  if(elementType == TOKEN_TYPE_UNDEFINED) return(false);
  
  t_symbol_type arrayType = get_corresponding_array_type(elementType);
  switch((u32)arrayType) {
    case(TYPE_ARRAY_INTEGER): {
      target->value_ai.len = arrayCount;
      target->value_ai.ptr = malloc(target->value_ai.len * sizeof(u32));
    } break;
    case(TYPE_ARRAY_FLOAT): {
      target->value_af.len = arrayCount;
      target->value_af.ptr = malloc(target->value_af.len * sizeof(r32));
    } break;
    case(TYPE_ARRAY_STRING): {
      target->value_as.len = arrayCount;
      target->value_as.ptr = malloc(target->value_as.len * sizeof(t_string));
    } break;
    default: assert(0);
  }
  target->type = arrayType;
  
  for(u32 arrayIndex = 0; arrayIndex < arrayCount; arrayIndex += 1) {
    t_token* element = array + arrayIndex;
    t_token_type currentType = get_token_source_type(symbols, element);
    if(currentType != elementType) return(false);
  }
  
  for(u32 arrayIndex = 0; arrayIndex < arrayCount; arrayIndex += 1) {
    t_token* element = array + arrayIndex;
    
    switch((u32)elementType) {
      case(TOKEN_TYPE_INTEGER): {
        target->value_ai.ptr[arrayIndex] = (u32)token_parse_integer(element);
      } break;
      case(TOKEN_TYPE_FLOAT): {
        target->value_af.ptr[arrayIndex] = (r32)token_parse_float(element);
      } break;
      case(TOKEN_TYPE_STRING): {
        target->value_as.ptr[arrayIndex] = string_copy_mem(token_parse_string(element));
      } break;
      case(TOKEN_TYPE_IDENTIFIER): {
        if(!write_token_value_to_symbol(symbols, target, element)) return(false);
      } break;
      default: assert(0);
    }
  }
  return(true);
}

internal bool parse_config_file(t_symbol_table* symbols, t_file_data fileData) {
  t_token_list tokens = lex_config_file(fileData);
  u32 tokenIndex = 0;
  loop {
    if(tokenIndex >= tokens.count) break;
    t_token* name = tokens.v + tokenIndex;
    tokenIndex += 1;
    
    if(tokenIndex >= tokens.count) goto error;
    t_token* op = tokens.v + tokenIndex;
    tokenIndex += 1;
    
    if(tokenIndex >= tokens.count) goto error;
    t_token* value = tokens.v + tokenIndex;
    tokenIndex += 1;
    
    if(op->type != TOKEN_TYPE_ASSIGMENT) goto error;
    if(name->type != TOKEN_TYPE_IDENTIFIER) goto error;
    
    bool overwrite = true;
    t_symbol* target = symbol_find_by_token(symbols, name);
    if(!target) {
      overwrite = false;
      target = symbol_create_from_token(symbols, name);
    }
    
    bool assigned = false;
    if(value->type >= TOKEN_TYPE_IDENTIFIER && value->type <= TOKEN_TYPE_STRING) {
      assigned = write_token_value_to_symbol(symbols, target, value);
    }
    else if(value->type == TOKEN_TYPE_ARRAY_OPEN) {
      if(tokenIndex >= tokens.count) goto error;
      t_token* arrayFirst = tokens.v + tokenIndex;
      t_token* arrayValue = arrayFirst;
      tokenIndex += 1;
      
      u32 arrayCount = 0;
      loop {
        if(arrayValue->type == TOKEN_TYPE_ARRAY_CLOSE) break;
        if(tokenIndex >= tokens.count) goto error;
        arrayValue = tokens.v + tokenIndex;
        tokenIndex += 1;
        arrayCount += 1;
      }
      assigned = write_token_array_to_symbol(symbols, target, arrayFirst, arrayCount);
    }
    
    if(!assigned) {
      if(!overwrite) {
        symbols->count -= 1;
      }
      goto error;
    }
  }
  
  return(true);
  error: return(false);
}

internal void config_free_symbols(t_symbol_table* symbols) {
  for(u32 symbolIndex = 0; symbolIndex < symbols->count; symbolIndex += 1) {
    t_symbol* symbol = symbols->v + symbolIndex;
    if (symbol->type == TYPE_STRING) {if(symbol->value_s.ptr) free(symbol->value_s.ptr); }
    else if(symbol->type == TYPE_ARRAY_INTEGER) {if(symbol->value_ai.ptr) free(symbol->value_ai.ptr); }
    else if(symbol->type == TYPE_ARRAY_FLOAT) {if(symbol->value_af.ptr) free(symbol->value_af.ptr); }
    else if(symbol->type == TYPE_ARRAY_STRING) {
      if(symbol->value_as.ptr) {
        for(u32 stringIndex = 0; stringIndex < symbol->value_as.len; stringIndex += 1) {
          free(symbol->value_as.ptr[stringIndex].ptr);
        }
        free(symbol->value_as.ptr);
      }
    }
  }
  free(symbols->v);
}

struct {
  t_symbol_type type;
  t_string name;
  union {
    i64* value_i;
    r64* value_f;
    t_string* value_s;
    t_array_i* value_ai;
    t_array_f* value_af;
    t_array_s* value_as;
  };
} typedef t_symbol_link;

struct {
  t_symbol_link* v;
  u32 count;
  u32 alloc;
} typedef t_link_list;

internal void link_add(t_link_list* list, t_symbol_link link) {
  if(list->count + 1 > list->alloc) {
    list->alloc *= 2;
    if(list->alloc == 0) list->alloc = 1;
    list->v = realloc(list->v, list->alloc * sizeof(t_symbol_link));
  }
  list->v[list->count] = link;
  list->count += 1;
}

internal void link_free(t_link_list* links) {
  free(links->v);
}

internal void link_create_i(t_link_list* list, t_string name, i64* value) {
  t_symbol_link result = {0};
  result.type = TYPE_INTEGER;
  result.name = name;
  result.value_i = value;
  link_add(list, result);
}

internal void link_create_f(t_link_list* list, t_string name, r64* value) {
  t_symbol_link result = {0};
  result.type = TYPE_FLOAT;
  result.name = name;
  result.value_f = value;
  link_add(list, result);
}

internal void link_create_s(t_link_list* list, t_string name, t_string* value) {
  t_symbol_link result = {0};
  result.type = TYPE_STRING;
  result.name = name;
  result.value_s = value;
  link_add(list, result);
}

internal void link_create_ai(t_link_list* list, t_string name, t_array_i* value) {
  t_symbol_link result = {0};
  result.type = TYPE_ARRAY_INTEGER;
  result.name = name;
  result.value_ai = value;
  link_add(list, result);
}

internal void link_create_af(t_link_list* list, t_string name, t_array_f* value) {
  t_symbol_link result = {0};
  result.type = TYPE_ARRAY_FLOAT;
  result.name = name;
  result.value_af = value;
  link_add(list, result);
}

internal void link_create_as(t_link_list* list, t_string name, t_array_s* value) {
  t_symbol_link result = {0};
  result.type = TYPE_ARRAY_STRING;
  result.name = name;
  result.value_as = value;
  link_add(list, result);
}

internal void config_initialize_links(t_symbol_table* symbols, t_link_list* links) {
  for(u32 symbolIndex = 0; symbolIndex < symbols->count; symbolIndex += 1) {
    t_symbol* symbol = symbols->v + symbolIndex;
    for(u32 linkIndex = 0; linkIndex < links->count; linkIndex += 1) {
      t_symbol_link* link = links->v + linkIndex;
      
      if(string_compare(link->name, symbol->name)) {
        // TODO(bumbread): make sure link type is an array type!!!
        if(link->type == symbol->type 
           || (symbol->type == TYPE_INTEGER && symbol->value_ai.len == 0)) {
          switch(link->type) {
            case(TYPE_INTEGER): {
              *link->value_i = symbol->value_i;
            } break;
            case(TYPE_FLOAT): {
              *link->value_f = symbol->value_f;
            } break;
            case(TYPE_STRING): {
              *link->value_s = string_copy_mem(symbol->value_s);
            } break;
            case(TYPE_ARRAY_INTEGER): {
              link->value_ai->len = symbol->value_ai.len;
              if(link->value_ai->ptr) free(link->value_ai->ptr);
              link->value_ai->ptr = malloc(link->value_ai->len * sizeof(u32));
              for(u32 valueIndex = 0; valueIndex < symbol->value_ai.len; valueIndex += 1) {
                link->value_ai->ptr[valueIndex] = symbol->value_ai.ptr[valueIndex];
              }
            } break;
            case(TYPE_ARRAY_FLOAT): {
              link->value_af->len = symbol->value_af.len;
              if(link->value_af->ptr) free(link->value_af->ptr);
              link->value_af->ptr = malloc(link->value_af->len * sizeof(r32));
              for(u32 valueIndex = 0; valueIndex < symbol->value_af.len; valueIndex += 1) {
                link->value_af->ptr[valueIndex] = symbol->value_af.ptr[valueIndex];
              }
            } break;
            case(TYPE_ARRAY_STRING): {
              link->value_as->len = symbol->value_as.len;
              if(link->value_as->ptr) free(link->value_as->ptr);
              link->value_as->ptr = malloc(link->value_as->len * sizeof(t_string));
              for(u32 valueIndex = 0; valueIndex < symbol->value_as.len; valueIndex += 1) {
                link->value_as->ptr[valueIndex] = string_copy_mem(symbol->value_as.ptr[valueIndex]);
              }
            } break;
          }
        }
        else {
          // TODO(bumbread): logging
        }
      }
      
    }
  }
}

internal t_string write_int_to_string(i64 value) {
  assert(value >= 0);
  t_string result = {0};
  do {
    string_append_char(&result, (value % 10) + '0');
    value /= 10;
  } while(value != 0);
  string_reverse(&result);
  return(result);
}

internal t_string write_float_to_string(r64 value) {
  assert(value > 0);
  i64 wholePart = (i64)value; // TODO(bumbread): this assumes the rounding method used by the compiler
  r64 remainder = value - (r64)((i64)wholePart);
  t_string result = write_int_to_string(wholePart);
  for(u32 digit = 0; digit < 6; digit += 1) {
    remainder *= 10.0;
    string_append_char(&result, ((char)remainder) + '0');
    remainder = remainder - (r64)((i64)remainder); // TODO(bumbread): make this more clear
  }
  return(result);
}

// TODO(bumbread): fix memory leak. some of these results have zero length
// at the initialization, causing string_append* functions to call realloc
// on pointer that wasn't previously allocated, causing it to call malloc
// and the value of the string gets thrown away after it is assigned.

internal t_string write_string_to_string(t_string value) {
  t_string result = {0};
  string_append_char(&result, '"');
  string_append(&result, value);
  string_append_char(&result, '"');
  return(result);
}

internal t_string write_int_array_to_string(t_array_i value) {
  t_string result = {0};
  string_append_char(&result, '{');
  for(u32 index = 0; index < value.len; index += 1) {
    t_string number = write_int_to_string(value.ptr[index]);
    string_append(&result, number);
    if(index != value.len - 1) {
      string_append_char(&result, ',');
      string_append_char(&result, ' ');
    }
  }
  string_append_char(&result, '}');
  return(result);
}

internal t_string write_float_array_to_string(t_array_f value) {
  t_string result = {0};
  string_append_char(&result, '{');
  for(u32 index = 0; index < value.len; index += 1) {
    t_string number = write_float_to_string((r64)value.ptr[index]);
    string_append(&result, number);
    if(index != value.len - 1) {
      string_append_char(&result, ',');
      string_append_char(&result, ' ');
    }
  }
  string_append_char(&result, '}');
  return(result);
}

internal t_string write_string_array_to_string(t_array_s value) {
  t_string result = {0};
  string_append_char(&result, '{');
  for(u32 index = 0; index < value.len; index += 1) {
    t_string number = write_string_to_string(value.ptr[index]);
    string_append(&result, number);
    if(index != value.len - 1) {
      string_append_char(&result, ',');
      string_append_char(&result, ' ');
    }
  }
  string_append_char(&result, '}');
  return(result);
}

internal void app_write_config_links_to_file(t_link_list* links, t_string16 filename) {
  t_file_data output = {0};
  output.filename = filename;
  
  t_string outputBuffer = {0};
  for(u32 linkIndex = 0; linkIndex < links->count; linkIndex += 1) {
    t_symbol_link* link = links->v + linkIndex;
    string_append(&outputBuffer, link->name);
    string_append(&outputBuffer, char_count("="));
    
    t_string value = {0};
    switch(link->type) {
      case(TYPE_INTEGER): value = write_int_to_string(*link->value_i); break;
      case(TYPE_FLOAT): value = write_float_to_string(*link->value_f); break;
      case(TYPE_STRING): value = write_string_to_string(*link->value_s); break;
      case(TYPE_ARRAY_INTEGER): value = write_int_array_to_string(*link->value_ai); break;
      case(TYPE_ARRAY_FLOAT): value = write_float_array_to_string(*link->value_af); break;
      case(TYPE_ARRAY_STRING): value = write_string_array_to_string(*link->value_as); break;
    }
    string_append(&outputBuffer, value);
    string_append_char(&outputBuffer, ';');
    string_append_char(&outputBuffer, '\n');
  }
  
  output.size = (u64)outputBuffer.len;
  output.ptr = outputBuffer.ptr;
  platform_write_file(output);
  free(outputBuffer.ptr);
}

internal void app_load_config(t_app_config* appConfig, t_string16 filename) {
  debug_variable_unused(appConfig);
  
  config_load_default(appConfig);
  
  t_file_data configData = platform_load_file(filename);
  t_symbol_table symbols = {0};
  bool shouldWriteNewConfig = (configData.ptr == 0);
  if(configData.ptr) {
    parse_config_file(&symbols, configData);
  }
  
  t_link_list links = {0};
  link_create_ai(&links, char_count("color_cycle"), &appConfig->colorCycle);
  link_create_i(&links, char_count("use_linear_interpolation"), &appConfig->useInterpolation);
  config_initialize_links(&symbols, &links);
  config_free_symbols(&symbols);
  
  if(shouldWriteNewConfig) {
    app_write_config_links_to_file(&links, filename);
  }
  
  link_free(&links);
}
