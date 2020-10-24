
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
  TYPE_INTEGER,
  TYPE_STRING,
  TYPE_FLOAT,
  TYPE_IDENTIFIER,
  
  TYPE_LPAREN,
  TYPE_RPAREN,
  TYPE_LARR,
  TYPE_PARR,
  
  TYPE_OPERATOR,
  
  TYPE_ERROR,
  TYPE_EOF,
};

struct t_token {
  u32 start;
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

internal void in_range(char x, char y, char c) {
  return(c >= x && c <= y);
}

internal void is_operator(char c) {
  return(c=='+' || c=='-' || c=='*' || c=='/' || c=='=');
}

internal struct t_token_list app_lex_config(struct t_app_config* appConfig, t_file_data data) {
  char* characters = (char*)data->ptr;
  u32 pos = 0;
  struct t_token currentToken = {0};
  struct t_token_list list = {0};
  
  state_main: {
    currentToken.pos = pos;
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

internal bool app_parse_config(struct t_token_list tokenList, t_app_config* appConfig) {
  
}

internal void app_load_config(struct t_app_config* appConfig, t_string16 filename) {
  debug_variable_unused(appConfig);
  app_load_default_config(appConfig);
  
  t_file_data configData = platform_load_file(filename);
  if(configData.size) {
    struct t_app_config tempConfig = {0};
    t_token_list configTokens = app_lex_config(&tempConfig);
    if(configTokens.error) {
      // TODO(bumbread): error handle this
    }
    else {
      *appConfig = tempConfig;
    }
  }
  else {
    app_write_config_to_file(appConfig, filename);
  }
}
