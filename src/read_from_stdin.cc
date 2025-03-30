#include <iostream>
#include <list>
#include <optional>
#include <thread>

#include <winsock2.h>
#include <windows.h>

/**
 * reads from stdin with timeout, or nonblocking.
 * Depending whatever the input source is (console, pipe or file)
 */
static bool read_available_data_from_stdin( std::list<int> & msg )
{
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

    if( hStdin == INVALID_HANDLE_VALUE ) {
        std::cerr <<  "INVALID_HANDLE_VALUE";
        return false;
    }

    DWORD fileType = GetFileType(hStdin);

    // 1) handle pipes 
    if (fileType == FILE_TYPE_PIPE) {
        unsigned int length = 0;
        DWORD bytesAvailable = 0;
        BOOL success = PeekNamedPipe(hStdin,NULL,0,NULL,&bytesAvailable,NULL);

        if( !success ) {
            // pipe is closed
            if( msg.empty() || msg.back() != EOF ) {
                msg.push_back( EOF );
            }
            return false;
        }

        if(bytesAvailable > 0) {
            for (int i = 0; i < bytesAvailable; i++) {
                char c = getchar();
                msg.push_back( c );
            }

            return true;
        }

        return false;
    }

    // handle character sources like console input
    if( fileType == FILE_TYPE_CHAR ) {
        HANDLE eventHandles[] = { hStdin };

        DWORD result = WSAWaitForMultipleEvents(sizeof(eventHandles)/sizeof(eventHandles[0]),
                                                &eventHandles[0],
                                                FALSE,
                                                10,
                                                TRUE );

        if( result == WSA_WAIT_EVENT_0 + 0 ) { // stdin at array index 0
            DWORD cNumRead;
            INPUT_RECORD irInBuf[128];
            int counter=0;

            if( ReadConsoleInput( hStdin, irInBuf, std::size(irInBuf), &cNumRead ) ) {

                // cNumRead is the number of characters available in the current input buffer
                // we can read without blocking                
                for( unsigned i = 0; i < cNumRead; ++i ) {
                    switch(irInBuf[i].EventType)
                    {
                    case KEY_EVENT:
                        if( irInBuf[i].Event.KeyEvent.bKeyDown ) {

                            // but filter out only ascii characters                            
                            char c = irInBuf[i].Event.KeyEvent.uChar.AsciiChar;
                            if( c && isascii( c ) ) {
                                // echo on
                                std::cout << c;
                                msg.push_back( c );
                            } // if
                        } // if
                    } // switch
                } // for

                return true;
            } // if
        } // if
        return false;
    } // if fileType == FILE_TYPE_CHAR

    // file is a regular file on disc
    if( fileType == FILE_TYPE_DISK  ) {
        char buffer[100];
        DWORD NumberOfBytesRead = 0;
        if( ReadFile( hStdin, &buffer, sizeof(buffer), &NumberOfBytesRead, NULL ) ) {

        	// https://learn.microsoft.com/en-us/windows/win32/fileio/testing-for-the-end-of-a-file
        	if( NumberOfBytesRead == 0 ) {
        		msg.push_back( EOF );
        		return false;
        	}

            for( unsigned i = 0; i < NumberOfBytesRead; ++i ) {
                msg.push_back( buffer[i] );
            }            
            return true;
        } else {
            return false;
        }
    }

    return false;
}

std::optional<char> get_char( std::chrono::steady_clock::duration duration = std::chrono::milliseconds(10) )
{
    static std::list<int> data;

    for( ;; std::this_thread::sleep_for(duration) ) {

        read_available_data_from_stdin( data );

        if( data.empty() ) {
            continue;
        }

        int c = data.front();
        data.pop_front();

        return static_cast<char>(c);
    }

    return {};
}


int main()
{
    while( true ) {
        std::optional<char> c = get_char();

        // end of file reached?
        if( c && c == EOF ) {
            break;
        }

        if( c ) {
        	if( c == '\r' ) {
        		std::cout << std::endl;
        	} else {
        		std::cout << *c;
        	}
        }
    } // while

    return 0;
}
