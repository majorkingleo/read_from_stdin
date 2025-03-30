#include <iostream>
#include <list>
#include <optional>
#include <thread>

#ifdef _WIN32
#   include <winsock2.h>
#   include <windows.h>
#else
#   include <sys/select.h>
#   include <unistd.h>
#endif

#ifdef _WIN32
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
#else

#ifdef __CYGWIN__
# ifndef STDIN_FILENO
#   define STDIN_FILENO 0
#  endif
#endif

static bool read_available_data_from_stdin( std::list<int> & msg )
{
    // timeout structure passed into select
    struct timeval tv;
    // fd_set passed into select
    fd_set fds;
    // Set up the timeout.  here we can wait for 1 second
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    // Zero out the fd_set - make sure it's pristine
    FD_ZERO(&fds);
    // Set the FD that we want to read
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    // select takes the last file descriptor value + 1 in the fdset to check,
    // the fdset for reads, writes, and errors.  We are only passing in reads.
    // the last parameter is the timeout.  select will return if an FD is ready or
    // the timeout has occurred
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    // return 0 if STDIN is not ready to be read.
    if( !FD_ISSET(STDIN_FILENO, &fds) ) {
        return false;
    }

    std::string message;
    std::getline( std::cin, message );

    if( !message.empty() ) {
        for( unsigned i = 0; i < message.size(); ++i ) {
            msg.push_back( message[i] );
        }
        msg.push_back( '\n' );
    }


    if( std::cin.eof() ) {
        if( msg.empty() || msg.back() != EOF ) {
            msg.push_back( EOF );
        }
    }

    return true;
}

#endif


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

        	if( *c == '\r' ) {
        		std::cout << std::endl;
        	} else {
        		std::cout << *c;
        	}
        }
    } // while

    return 0;
}
