/*******************************************************************************
*
* Simple Serial Terminal for Arduino
* Designed for esp8266 or boards with sufficient memory
*
* Author: Cyrill v.W. <eni e23.ch>
*
*******************************************************************************/

#ifndef SerialTerm_h
#define SerialTerm_h

#include <Arduino.h>
#include <stdarg.h>


#ifndef TTY_BUFFER_SIZE
	#define TTY_BUFFER_SIZE 128
#endif

#ifndef TTY_MAX_CMDS
	#define TTY_MAX_CMDS 16
#endif


// Terminal escape sequences
#define TTY_OFF             "\x1b[0m"
#define TTY_BOLD            "\x1b[1m"
#define TTY_LOWINTENS       "\x1b[2m"
#define TTY_UNDERLINE       "\x1b[4m"
#define TTY_BLINK           "\x1b[5m"
#define TTY_REVERSE         "\x1b[7m"
#define TTY_COLOR_RED 			"\x1b[0;31m"
#define TTY_COLOR_GREEN 		"\x1b[0;32m"
#define TTY_COLOR_ORANGE 		"\x1b[0;33m"
#define TTY_COLOR_BLUE 			"\x1b[0;34m"
#define TTY_COLOR_PURPLE 		"\x1b[0;35m"
#define TTY_COLOR_CYAN 			"\x1b[0;36m"
#define TTY_COLOR_GRAY 			"\x1b[0;37m"
#define TTY_COLOR_YELLOW 		"\x1b[1;33m"
#define TTY_COLOR_RST 			"\x1b[0m"
#define TTY_CLS							"\x1b[2J"
#define TTY_CURSOR_XY(x,y)	"\x1b[x;yH"
#define TTY_CURSOR_UP(x)  	"\x1b[xA"
#define TTY_CURSOR_DOWN(x)  "\x1b[xB"
#define TTY_CURSOR_RIGHT(x) "\x1b[xC"
#define TTY_CURSOR_LEFT(x)  "\x1b[xD"
#define TTY_CURSOR_HOME     "\x1b[H"



typedef void (*SerialTermCallback) ();



class SerialTerm {

	private:
	// buffer for line input
	char buffer[TTY_BUFFER_SIZE];
	// current position in the buffer
	int buffer_pos;
	// next free callback slot
	int callback_pos;
	// cmd callback name lookup table
	String lookup_table[TTY_MAX_CMDS];
	// Array with the callbacks itself
	SerialTermCallback callbacks[TTY_MAX_CMDS];
	// last line, used for getArg functions
	String last_line;

	/**
	 * rcv_char() is getting called for every char recived by serial
	 **/
	void rcv_char(){
		char current_char = Serial.read();
		// backspace: delete last char in buffer
  	if ( current_char == 0x7F && buffer_pos > 0 ) {
    	buffer[buffer_pos-1];
    	buffer_pos--;
    	Serial.print("\b");
    	Serial.print(" ");
    	Serial.print("\b");
    	return;
  	}
  	// carriage return recived = new line
  	if (current_char == 13) {
    	Serial.println( current_char );
    	buffer[buffer_pos] = '\0';
			last_line = String(buffer);
    	parse_line();
    	return;
  	}
		// TAB
		if (current_char == '\t') {
			tab_completion();
			return;
		}
		// up && down && l && r
		/*if ( (current_char == 65) || // up
				 (current_char == 66) || // down
				 (current_char == 67) || // left
				 (current_char == 68) ){ // right
			return;
		}*/

		// put every other char than cr into buffer
  	Serial.print( current_char );
  	buffer[buffer_pos] = current_char;
  	buffer_pos++;
	}


	/**
	 * parse_line() gets called when rcv_char() detects a newline
	 **/
	void parse_line(){
		// split input line by spaces and take the first item as command
	  String command = "";
	  command = str_token( last_line, ' ', 0 );
		if (command.endsWith(" ")){
			command = command.substring(0, command.length()-1);
		}

	  // empty buffer and zero counter
	  memset(buffer, 0, sizeof( buffer ) );
	  buffer_pos = 0;

	  // simple pong for recognition
	  if (  command ==  "ping" ) {
	    Serial.println( "pong" );
	    return;
	  }
		// catch empty command
		if ( command == ""){
			return;
		}

		for (int i=0; i<TTY_MAX_CMDS; i++){

			if (command==lookup_table[i]){
				callbacks[i]();
				return;
			}
		}

		// if nothing matches, command not found
		debug("unknown command");
		printf("last_line '%s'\n\r", str2char(last_line) );
		return;
	}


	String get_buff_input(){
		String input = String("");
		if (buffer_pos>0){
			char tb[buffer_pos+1];
			for (int i=0; i<buffer_pos;i++){
				tb[i] = buffer[i];
			}
			tb[buffer_pos+1]='\0';
			input = String(tb);
		}
		return input;
	}


	//
	// Experimential: command tab completion
	//
	void tab_completion() {

		String input = get_buff_input();
		// return if function arg
		String s_chk = str_token(input, ' ', 1);
		if (s_chk.length()>0){
			return;
		}
		// count matches and get last one
		int cmdc = 0;
		int last_match = 0;
		for (int i=0; i<TTY_MAX_CMDS; i++){
			if (lookup_table[i].startsWith(input)){
				cmdc++;
				last_match = i;
			}
		}
		// nothing matches: return
		if (cmdc==0){
			return;
		}
		// 1 match: complete command
		if (cmdc==1){
			String pstr = lookup_table[last_match].substring(input.length());
			// fill buffer
			for (int i=0; i<pstr.length();i++){
				buffer[buffer_pos] = pstr.charAt(i);
				buffer_pos++;
			}
			buffer[buffer_pos] = ' ';
			buffer_pos++;
			Serial.print(pstr);
			Serial.print(" ");
			return;
		}
		// more than 1 match: print commands
		// TODO: complete to same
		if (cmdc>1) {
			Serial.print("\x1b\n\r");
			//int reslen = 0;
			for (int i=0; i<TTY_MAX_CMDS; i++){
				if (lookup_table[i].startsWith(input)){
					//reslen+=(lookup_table[i].length()+4);
					Serial.print(lookup_table[i]);
					Serial.print('\t');
				}
			}
			Serial.print("\n\r");
			Serial.print(input);
		}
	}


  public:

	boolean debug_enabled = true;

	/**
	 * Constructor
	 **/
  SerialTerm() {
		buffer_pos = 0;
		callback_pos = 0;
		last_line = "";
  }

	/**
	 * Begin Serial
	 **/
	void begin(int TTY_baud){
		Serial.begin( TTY_baud );
	}


	/**
	 * printf to terminal
	 * This function does not support String since it uses variadic
	 **/
	void printf(char *fmt, ... ){
	  char buf[256];
	  va_list args;
	  va_start (args, fmt );
	  vsnprintf(buf, 256, fmt, args);
	  va_end (args);
	  Serial.print(buf);
	}


	/**
	 * printf with an color
	 **/
	void printf_color(char* color, char *fmt, ... ){
	  char buf[256]; // resulting string limited to 256 chars
	  va_list args;
	  va_start (args, fmt );
	  vsnprintf(buf, 256, fmt, args);
	  va_end (args);
		printf("\033[%sm", color );
		Serial.print(buf);
		printf("\033[%sm", TTY_COLOR_RST );
	}


	/**
	 * Read raw binary of fixed size
	 **/
	uint8_t* read_binary(int len){
		uint8_t chbuffer[len+1];
    int cc = 0;
    delay(100);
    while (cc<len){
      chbuffer[cc] = Serial.read();
      cc++;
    }
		return chbuffer;
	}


	/**
	 * Print a message this->debug_enabled is set to true
	 **/
	void debug( const char* message ) {
	  if ( debug_enabled ) {
	   Serial.println( message );
	  }
	}


	/**
	 * Get single lastline argument as integer
	 **/
	int intArg(int index){
	  String s_str = "";
	  s_str = strArg( index );
	  int s;
	  s = s_str.toInt();
	  return s;
	}


	/**
	 * Get single lastline argument as float
	 **/
	float floatArg(int index){
	  String s_str = "";
	  s_str = strArg( index );
	  float s;
	  s = s_str.toFloat();
	  return s;
	}


	/**
	 * Get single lastline argument as char array
	 **/
	char* charArg(int index){
		String s_str = "";
		s_str = str_token(last_line, ' ', index+1);
		char charbuff[s_str.length()+1];
		s_str.toCharArray(charbuff, s_str.length()+1);
		return charbuff;
	}


	char* str2char(String s){
		char charbuff[s.length()+1];
		s.toCharArray(charbuff, s.length()+1);
		return charbuff;
	}


	/**
	 * Get single lastline argument as string
	 **/
	String strArg(int index){
		return str_token(last_line, ' ', index+1);
	}


	/**
	 * split a string and get chuck at desired position
	 * example:
	 * data="foo,bar,baz" separator="," index=2  => baz
	 * data="foo/bar" separator="/" index=0  => foo
	 **/
	String str_token( String data, char separator, int index ) {
	  int found = 0;
	  int strIndex[] = { 0, -1 };
	  int maxIndex = data.length()-1;
	  for( int i = 0; i <= maxIndex && found <= index; i++ ){
	    if( data.charAt( i ) == separator || i == maxIndex ){
	        found++;
	        strIndex[0] = strIndex[1] + 1;
	        strIndex[1] = ( i == maxIndex ) ? i + 1 : i;
	    }
	  }
	  return found > index ? data.substring( strIndex[0], strIndex[1] ) : "";
	}


	/**
	 * Set an callback for a command
	 **/
	void on(String cmd, SerialTermCallback func){
		lookup_table[callback_pos] = cmd;
		callbacks[callback_pos] = func;
		callback_pos++;
	}


	/**
	 * Poll function, needs to run in main loop
	 **/
  void run(){
		if ( Serial.available() ) {
    	rcv_char();
  	}
  }


};

#endif
