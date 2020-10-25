
internal t_file_data app_write_config_to_file(struct t_app_config* appConfig, t_string16 filename) {
  
#if 0
  t_file_data output;
  output.filename = filename;
  platform_write_file(output);
#endif
}

internal void app_load_default_config(struct t_app_config* appConfig) {
  appConfig->error = false;
  appConfig->backgroundColor = 0xffff00ff; // test
}

enum t_token_type {
  TYPE_ERROR,
  
  TYPE_INTEGER,
  TYPE_STRING,
  TYPE_FLOAT,
  TYPE_IDENTIFIER,
  
  TYPE_LPAREN,
  TYPE_RPAREN,
  TYPE_LARR,
  TYPE_PARR,
  
  TYPE_OPERATOR,
  
  TYPE_EOF,
};

struct t_token {
  char* ptr;
  u32 len;
  enum t_token_type type;
};

struct t_token_list {
  bool error;
  u32 tokensCount;
  u32 tokensAlloc;
  struct t_token* tokens;
};

internal void token_push(struct t_token_list* list, struct t_token token) {
  if(list->tokensCount + 1 > list->tokensAlloc) {
    list->tokensAlloc *= 2;
    if(list->tokensAlloc == 0) list->tokensAlloc = 1;
    list->tokens = realloc(list->tokens, list->tokensAlloc * sizeof(struct t_token));
  }
  list->tokens[list->tokensCount] = token;
  list->tokensCount += 1;
}

internal void token_pop(struct t_token_list* list, u32 index) {
  list->tokensCount -= 1;
  list->tokens[list->tokensCount] = {0};
}

internal void token_list_free(struct t_token_list list) {
  if(list->tokens != 0) {
    free(list->tokens);
  }
}

internal void in_range(char x, char y, char c) {
  return(c >= x && c <= y);
}

internal void is_operator(char c) {
  return(c=='+' || c=='-' || c=='*' || c=='/' || c=='=');
}

internal struct t_token_list config_lex(struct t_app_config* appConfig, t_file_data data) {
  char* characters = (char*)data->ptr;
  u32 pos = 0;
  struct t_token currentToken = {0};
  struct t_token_list list = {0};
  
  state_main: {
    currentToken.ptr = characters + pos;
    char currentChar = characters[pos];
    
    if(currentChar == 0) {
      pos += 1;
      currentToken.len += 1;
      token_push(&list, currentToken);
      goto exit;
    }
    else if(currentChar == '0') {
      currentToken.type = TYPE_INTEGER;
      pos += 1;
      currentToken.len += 1;
      goto state_integer_prefix;
    }
    else if(in_range('1', '9', currentChar)) {
      currentToken.type = TYPE_INTEGER;
      pos += 1;
      currentToken.len += 1;
      goto state_integer_dec;
    }
    else if(in_range('a', 'z', currentChar)
            || in_range('A', 'Z', currentChar)
            || currentChar == '_') {
      pos += 1;
      currentToken.len += 1;
      currentToken.type = TYPE_IDENTIFIER;
      goto state_identifier;
    }
    else if(currentChar == '"') {
      pos += 1;
      currentToken.len += 1;
      currentToken.type = TYPE_STRING;
      goto state_string;
    }
    else if(currentChar == '(') {
      pos += 1;
      currentToken.len += 1;
      currentToken.type = TYPE_LPAREN;
      token_push(&list, currentToken);
      goto state_main;
    }
    else if(currentChar == ')') {
      pos += 1;
      currentToken.len += 1;
      currentToken.type = TYPE_RPAREN;
      token_push(&list, currentToken);
      goto state_main;
    }
    else if(currentChar == '{') {
      pos += 1;
      currentToken.len += 1;
      currentToken.type = TYPE_LARR;
      token_push(&list, currentToken);
      goto state_main;
    }
    else if(currentChar == '(') {
      pos += 1;
      currentToken.len += 1;
      currentToken.type = TYPE_PARR;
      token_push(&list, currentToken);
      goto state_main;
    }
    else if(is_operator(currentChar)) {
      pos += 1;
      currentToken.len += 1;
      currentToken.type = TYPE_OPERATOR;
      token_push(&list, currentToken);
      goto state_main;
    }
    else {
      pos += 1;
      goto state_main;
    }
  }
  
  state_integer_prefix: {
    char currentChar = characters[pos];
    if(currentChar == 'x') {
      currentToken.len += 1;
      pos += 1;
      goto state_integer_hex;
    }
    else if(currentChar == 'b') {
      currentToken.len += 1;
      pos += 1;
      goto state_integer_bin;
    }
    else if(in_range('0', '9', currentChar)) {
      currentToken.len += 1;
      pos += 1;
      goto state_integer_dec;
    }
    else if(currentChar == '.') {
      currentToken.type = TYPE_FLOAT;
      currentToken.len += 1;
      pos += 1;
      goto state_float;
    }
    else {
      token_push(&list, currentToken);
      goto state_main;
    }
  }
  
  // TODO(bumbread): do i want 0x to mean 0 ???
  state_integer_hex: {
    char currentChar = characters[pos];
    if(0
       || in_range('0', '9', currentChar)
       || in_range('a', 'f', currentChar)
       || in_range('A', 'D', currentChar)) {
      currentToken.len += 1;
      pos += 1;
      goto state_integer_hex;
    }
    else {
      token_push(&list, currentToken);
      goto state_main;
    }
  }
  
  state_integer_bin: {
    char currentChar = characters[pos];
    if(in_range('0', '1', currentChar)) {
      currentToken.len += 1;
      pos += 1;
      goto state_integer_bin;
    }
    else {
      token_push(&list, currentToken);
      goto state_main;
    }
  }
  
  state_integer_dec: {
    char currentChar = characters[pos];
    if(in_range('0', '9', currentChar)) {
      currentToken.len += 1;
      pos += 1;
      goto state_integer_bin;
    }
    if(currentChar == '.') {
      currentToken.type = TYPE_FLOAT;
      currentToken.len += 1;
      pos += 1;
      goto state_float;
    }
    else {
      token_push(&list, currentToken);
      goto state_main;
    }
  }
  
  state_float: {
    char currentChar = characters[pos];
    if(currentChar == '.') {
      pos += 1;
      currentToken.len += 1;
      currentToken.type = TYPE_ERROR;
      goto lex_error;
    }
    else if(currentChar == 'e') {
      pos += 1;
      currentToken.len += 1;
      goto state_float_exponent;
    }
    else if(in_range('0', '9')) {
      pos += 1;
      currentToken.len += 1;
      goto state_float;
    }
    else {
      token_push(&list, currentToken);
      goto state_main;
    }
  }
  
  state_float_exponent: {
    char currentChar = characters[pos];
    if(currentChar == 'e') goto lex_error;
    else if(currentChar == '.') goto lex_error;
    else if(in_range('0', '9', currentChar)) {
      pos += 1;
      currentToken.len += 1;
      goto state_float_exponent;
    }
    else {
      token_push(&list, currentToken);
      goto state_main;
    }
  }
  
  state_identifier: {
    char currentChar = characters[pos];
    if(currentChar == '_'
       || in_range('a', 'z', currentChar)
       || in_range('A', 'Z', currentChar)
       || in_range('0', '9', currentChar)) {
      pos += 1;
      currentToken.len += 1;
      goto state_identifier;
    }
    else {
      token_push(&list, currentToken);
      goto state_main;
    }
  }
  
  state_string: {
    char currentChar = characters[pos];
    if(currentChar == '\\') {
      // TODO(bumbread): might lex_error on undefined screen
      pos += 2;
      currentToken.len += 2;
      goto state_string;
    }
    else if(currentChar == '"') {
      pos += 1;
      currentToken.len += 1;
      token_push(&list, currentToken);
      goto state_main;
    }
    else {
      pos += 1;
      currentToken.len += 1;
      goto state_string;
    }
  }
  
  // TODO(bumbread): error messages
  lex_error: {
    currentToken.len += 1;
    currentToken.type = TYPE_ERROR;
    list.error = true;
    return list;
  }
  
  exit: {
    return list;
  }
}

struct t_identifier {
  t_token name;
  t_token_type type;
  union {
    u64 value_i;
    float value_f;
    struct {
      u32 arrayLen;
      struct t_token* array;
    };
    struct {
      u32 stringLen;
      char* string;
    };
  };
};

struct t_identifier_list {
  t_identifier* identifiers;
  u32 count;
  u32 alloc;
  bool error;
};

internal struct t_identifier* identifier_push(struct t_identifier_list* list, struct t_identifier identifier) {
  if(list->count + 1 > list->alloc) {
    list->alloc *= 2;
    if(list->alloc == 0) list->alloc = 1;
    list->identifiers = realloc(list->identifiers, list->alloc * sizeof(struct t_identifier));
  }
  struct t_identifier* result = list->identifiers + list->count;
  *result = identifier;
  list->count += 1;
  return(result);
}

internal bool compare_tokens(struct t_token a, struct t_token b) {
  if(a.len != b.len) return(false);
  for(u32 i = 0; i < a.len; i += 1) {
    if(a.ptr[i] != b.ptr[i]) return(false);
  }
  return(true);
}

internal struct t_identifier* find_identifier(struct t_identifier_list* list, struct t_token name) {
  for(u32 i = 0; i < list.count; i += 1) {
    if(compare_tokens(list.identifiers[i].name, name)) {
      return(list.identifiers + i);
    }
  }
  return(0);
}

internal struct t_identifier_list config_parse_identifiers(struct t_token_list tokenList) {
  struct t_identifier_list identifiers = {0};
  
  u32 index = 0;
  loop {
    if(index >= tokenList.tokensCount) goto error;
    struct t_token name = tokenList.tokens[index];
    if(name.type != TYPE_IDENTIFIER) goto error;
    index += 1;
    
    struct t_identifier* target = find_identifier(&list, identifier.name);
    if(!target) {
      struct t_identifier identifier = {0};
      identifier.name = name;
      identifier.type = value.type;
      target = identifier_push(&list, identifier);
    }
    
    if(index >= tokenList.tokensCount) goto error;
    struct t_token assignmentOperator = tokenList.tokens[index];
    index += 1;
    if(assignmentOperator.type != TYPE_OPERATOR) goto error;
    if(assignmentOperator.len != 1) goto error;
    if(assignmentOperator.ptr[0] != '=')  goto error;
    
    if(index >= tokenList.tokensCount) goto error;
    struct t_token value = t_token assignmentOperator = tokenList.tokens[index];
    if(value.type < TYPE_INTEGER || value.type > TYPE_IDENTIFIER) goto error;
    index += 1;
    
    switch(value.type) {
      case(TYPE_INTEGER): {
        
        u64 base = 10;
        u64 result = 0;
        if(value.len >= 2) {
          if(value.ptr[1] == 'x') {
            base = 16;
          }
          else if(value.ptr[1] == 'b') {
            base = 2;
          }
        }
        for(u32 i = 0; i < value.len; i += 1) {
          char currentChar = value.ptr[i];
          u32 digit = 0;
          if(in_range('0', '9', currentChar)) {
            digit = (u32)(currentChar - '0');
          }
          else if(in_range('a', 'f', currentChar)) {
            digit = (u32)(currentChar - 'a');
          }
          else if(in_range('A', 'F', currentChar)) {
            digit = (u32)(currentChar - 'A');
          }
          else {
            assert(0);
          }
          result *= base;
          result += digit;
        }
        
        target->value_i = result;
        
      } break;
      
      case(TYPE_STRING): {
        
        if(value.len > 2) {
          target->string = malloc((value.len - 2) * sizeof(char));
          target->stringLen = value.len - 2;
          for(u32 i = 1; i < value.len - 1; i += 1) {
            char character = value.ptr[i];
            if(character == '\\') {
              i += 1;
              switch(value.ptr[i]) {
                case('n'): { character = '\n'; } break;
                case('t'): { character = '\t'; } break;
                case('"'): { character = '\"'; } break;
              }
            }
            target->string[i-1] = value.ptr[i];
          }
        }
        else {
          target->stringLen = 0;
          target->string = 0;
        }
        
      } break;
      
      case(TYPE_FLOAT): {
        
        float result = 0.0;
        float pw = 1.0f;
        
        u32 i = 0;
        for(;i < value.len; i += 1) {
          char digitChar = value.ptr[i];
          u32 digit = 0;
          if(in_range('0', '9', digitChar)) {
            digit = digitChar - '0';
          }
          else if(digitChar == '.') {
            break;
          }
          else if(digitChar == 'e' || digitChar == 'E') {
            break;
          }
          else assert(0);
          result *= 10.0f;
          result += (float)digit;
        }
        
        if(value.ptr[i] = '.') {
          i += 1;
          for(;i < value.len; i += 1) {
            char digitChar = value.ptr[i];
            u32 digit = 0;
            if(in_range('0', '9', digitChar)) {
              digit = digitChar - '0';
            }
            else if(digitChar == '.') {
              assert(0);
            }
            else if(digitChar == 'e' || digitChar == 'E') {
              break;
            }
            else assert(0);
            result *= 10.0f;
            pw /= 10.0f;
            result += (float)digit;
          }
        }
        result *= pw;
        
        if(value.ptr[i] == 'e' || value.ptr[i] == 'E') {
          i += 1;
          u32 expSign = 1;
          if(value.ptr[i] == '+') {
            expSign = 1;
          }
          else if(value.ptr[i] == '-') {
            expSign = -1;
          }
          else assert(0);
          i += 1;
          
          u32 exp = 0;
          for(;i < value.len; i += 1) {
            char digitChar = value.ptr[i];
            u32 digit = 0;
            if(in_range('0', '9', digitChar)) {
              digit = digitChar - '0';
            }
            else if(digitChar == '.') {
              assert(0);
            }
            else if(digitChar == 'e' || digitChar == 'E') {
              assert(0);
            }
            else assert(0);
            exp *= 10;
            exp += digit;
          }
          
          if(sign = 1) {
            for(u32 j = 0; j < exp; j += 1) {
              result *= 10.0f;
            }
          }
          else if(sign = -1) {
            for(u32 j = 0; j < exp; j += 1) {
              result /= 10.0f;
            }
          }
        }
        
      } break;
      
      case(TYPE_IDENTIFIER): {
        struct t_identifier* source = find_identifier(&list, value);
        if(!source) goto error;
        target->type = source->type;
        switch(target->type) {
          case(TYPE_INTEGER): {
            target->value_i = source->value_i;
          } break;
          case(TYPE_FLOAT) {
            target->value_f = source->value_f;
          } break;
          case(TYPE_STRING) {
            // TODO(bumbread): maybe this should be copied over by reference
            target->string = source->string;
            target->stringLength = source->stringLength;
          } break;
          default: assert(0);
        }
        
      } break;
      
    }
  }
  
  return(identifiers);
  error: {
    identifiers.error = true;
    return(identifiers);
  }
}

internal void config_load_identifiers(struct t_identifier_list list, struct t_app_config appConfig) {
  // TODO(bumbread): 
}

internal void app_load_config(struct t_app_config* appConfig, t_string16 filename) {
  debug_variable_unused(appConfig);
  app_load_default_config(appConfig);
  
  t_file_data configData = platform_load_file(filename);
  if(configData.size) {
    t_token_list configTokens = app_lex_config(&tempConfig);
    if(configTokens.error) {
      // TODO(bumbread): error handle this
    }
    else {
      struct t_identifier_list list = config_parse_identifiers(configTokens);
      if(list.error) {
        // TODO(bumbread): error handle
      }
      else {
        config_load_identifiers(list, &appConfig);
      }
    }
  }
  else {
    app_write_config_to_file(appConfig, filename);
  }
}
