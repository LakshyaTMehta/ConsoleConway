// System headers
#include <windows.h>

// Standard library C-style
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <stdbool.h>


#define MAX_CONSOLE_WIDTH 200
#define MAX_CONSOLE_HEIGHT 100

#define ESC "\x1b"
#define CSI "\x1b["
#define WinCon_clrscr() printf( CSI "2J" ); printf( CSI "1;1f" ); 

void WinCon_drawPixel( SHORT x, SHORT y );

bool buffer_a[ MAX_CONSOLE_HEIGHT ][ MAX_CONSOLE_WIDTH ] = { false };
bool buffer_b[ MAX_CONSOLE_HEIGHT ][ MAX_CONSOLE_WIDTH ] = { false };
bool buffer_select = false;


bool EnableVTMode()
{
    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
    {
        return false;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode))
    {
        return false;
    }
    return true;
}


void ClearScreen()
{
  HANDLE                     hStdOut;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD                      count;
  DWORD                      cellCount;
  COORD                      homeCoords = { 0, 0 };

  hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
  if (hStdOut == INVALID_HANDLE_VALUE) return;

  /* Get the number of cells in the current buffer */
  if (!GetConsoleScreenBufferInfo( hStdOut, &csbi )) return;
  cellCount = csbi.dwSize.X *csbi.dwSize.Y;

  /* Fill the entire buffer with spaces */
  if (!FillConsoleOutputCharacter(
    hStdOut,
    (TCHAR) ' ',
    cellCount,
    homeCoords,
    &count
    )) return;

  /* Fill the entire buffer with the current colors and attributes */
  if (!FillConsoleOutputAttribute(
    hStdOut,
    csbi.wAttributes,
    cellCount,
    homeCoords,
    &count
    )) return;

  /* Move the cursor home */
  SetConsoleCursorPosition( hStdOut, homeCoords );
}


void PrintVerticalBorder()
{
    printf(ESC "(0"); // Enter Line drawing mode
    printf(CSI "104;93m"); // bright yellow on bright blue
    printf("x"); // in line drawing mode, \x78 -> \u2502 "Vertical Bar"
    printf(CSI "0m"); // restore color
    printf(ESC "(B"); // exit line drawing mode
}


COORD WinCon_getWindowDimension( void )
{
    COORD Size = {0} ;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        printf("\nCouldn't get the console handle. Quitting.\n");
        return Size;
    }

    CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;
    GetConsoleScreenBufferInfo(hOut, &ScreenBufferInfo);
    Size.X = ScreenBufferInfo.srWindow.Right - ScreenBufferInfo.srWindow.Left + 1;
    Size.Y = ScreenBufferInfo.srWindow.Bottom - ScreenBufferInfo.srWindow.Top + 1;
    return Size;
}


void WinCon_setCursor( SHORT x, SHORT y )
{
    printf(CSI "%u;%uH", y, x );
}


void WinCon_drawPixel( SHORT x, SHORT y )
{
    WinCon_setCursor( x, y );
    printf( CSI "107m " );
    printf( CSI "0m" );
}


void WinCon_show( bool buffer[ MAX_CONSOLE_HEIGHT ][ MAX_CONSOLE_WIDTH ] )
{
    COORD Size = WinCon_getWindowDimension();
    for( int i = 0; i < Size.Y; i++ )
      for( int j = 0; j < Size.X; j++ )
        if( true == buffer[i][j] )
          WinCon_drawPixel( j + 1, i + 1 );
}


void WinCon_getStartState()
{
    HANDLE hOut = GetStdHandle( STD_OUTPUT_HANDLE );
    CONSOLE_SCREEN_BUFFER_INFO con_info;
    WinCon_setCursor(1, 1);
    char key = '\0';
    while( 'q' != key )
    {
        switch( key )
        {
            case '\r':
              {
                  GetConsoleScreenBufferInfo( hOut, &con_info );
                  WinCon_drawPixel( con_info.dwCursorPosition.X + 1, con_info.dwCursorPosition.Y + 1 );
                  buffer_a[ con_info.dwCursorPosition.Y ][ con_info.dwCursorPosition.X ] = true;
              }
            case 'h':
              printf( CSI "1D" );
              break;
            case 'j':
              printf( CSI "1B" );
              break;
            case 'k':
              printf( CSI "1A" );
              break;
            case 'l':
              printf( CSI "1C" );
              break;
            default:
              break;
        }
        key = getch();
    }
}


int WinCon_countLiveNeighbours( bool buffer[ MAX_CONSOLE_HEIGHT ][ MAX_CONSOLE_WIDTH ], int x, int y )
{
    int count = 0;
    for( int i = (y-1); i <= (y+1); i++ )
        for( int j = (x-1); j <= (x+1); j++ )
        {
            if ((i == y && j == x) || (i < 0 || j < 0) || (i >= MAX_CONSOLE_HEIGHT || j >= MAX_CONSOLE_WIDTH))
                continue;
            
            else if (buffer[i][j] == true){
                count++;
            }
        }
    return count;
}


void WinCon_step()
{
    COORD Size = WinCon_getWindowDimension();
    if( buffer_select == false )
    {
      for( int i = 0; i < Size.Y; i++ )
          for( int j = 0; j < Size.X; j++ )
          {
              int nc = WinCon_countLiveNeighbours( buffer_a, j, i );
              if( ((nc == 2) || (nc == 3)) && buffer_a[i][j] == true )
                buffer_b[i][j] = true;
              else if( (nc == 3) && buffer_a[i][j] == false )
                buffer_b[i][j] = true;
              else
                buffer_b[i][j] = false;
          }
      buffer_select = true;
      WinCon_show(buffer_b);
    }
    else
    {
      for( int i = 0; i < Size.Y; i++ )
          for( int j = 0; j < Size.X; j++ )
          {
              int nc = WinCon_countLiveNeighbours( buffer_b, j, i );
              if( ((nc == 2) || (nc == 3)) && buffer_b[i][j] == true )
                buffer_a[i][j] = true;
              else if( (nc == 3) && buffer_b[i][j] == false )
                buffer_a[i][j] = true;
              else
                buffer_a[i][j] = false;
          }
      buffer_select = false;
      WinCon_show( buffer_a );
    }
}


int main ( void )
{
    bool fSuccess = EnableVTMode();
    if (!fSuccess)
    {
        printf("\nUnable to enter VT processing mode. Quitting.\n");
        return -1;
    }
    // printf( CSI "?25l" ); /* Hide Cursor */
    WinCon_clrscr()
    WinCon_getStartState();
    
    while( true )
    {
//      printf( CSI "2J" );
//      printf( CSI "1;1H" );
      ClearScreen();
      WinCon_step();
      Sleep(50);
    }

    return 0;
}
