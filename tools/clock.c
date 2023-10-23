#include <stdio.h>
#include <stdlib.h>

//#define MUL 1  // 8Mhz
#define MUL 2  // 16 MHz
//#define MUL 3  // 24 MHz

/** python3 plot.py

plot.py:

import numpy  as np
import matplotlib.pyplot as plt
data = np.loadtxt('data.txt')


x = data[:, 0]
y = data[:, 1]
plt.plot(x, y,'r--')
plt.show()


********************************************/



int main( int argc, char argv[] ){
    char *pName = "Clock.txt";
    int c, prevc;
    FILE * fIn;
    FILE * fView;
    int nbchar=0;
    int pulsewidth=0;
    int pulsecount=0;

    fIn = fopen ( pName, "r" );
    if ( fIn == NULL ){
        printf( "File not found: %s\n", pName );
        exit(1);
    }
    fView = fopen("data.txt", "w" );
    prevc=0;

    while ( 1 ){
        c = fgetc(fIn);
        if ( c == EOF ){
            printf( "\nEnd of file : %s pulsecount=%d\n", pName, pulsecount );
            break;
        }
        if ( (c != '0') && (c != '1') ){
            continue;
        }
        nbchar++;
        if ( pulsecount > 2000 ){
            printf( "\nStop reading %s -- %d\n", pName, pulsecount );
            break;
        }

        //putchar(c);
        if ( (prevc == '0') && (c == '1') ){
            pulsecount++;
        }
        //fputc('>', fView);
        if ( (prevc == '1') && (c == '0') ){
            if( pulsewidth > 100*MUL)
                fprintf(fView, "%d %d\n", pulsecount, 100*MUL );
            else
                fprintf(fView, "%d %d\n", pulsecount, pulsewidth );

            printf("\n");
            if ( pulsewidth > 62*MUL ){
              putchar('1');
            }
            else if ( pulsewidth > 58*MUL ){
              putchar('0');
            }
            else{
                putchar('x');
            }
            pulsewidth=0;
        }
        pulsewidth++;
        prevc = c;
    }

    fclose(fView);
    fclose(fIn);

}
