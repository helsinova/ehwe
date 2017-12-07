/***************************************************************************
 *   Copyright (C) 2017 by Michael Ambrus                                  *
 *   michael@helsinova.se                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/*
 * Routines for managing serial lines
 */

#include "config.h"
#include "serial.h"
#include <log.h>

#ifdef HAVE_POSIX_TERMIO
#include <termio.h>
#else
#error Only POSIX TERMIO supported by __FILE__
#endif

#include "config.h"
#include "serial.h"
#include <log.h>
#include <assure.h>
#include <string.h>

#define LOGLINE_MAX 500

#define BAUD_ALL ( \
    B0          | \
    B50         | \
    B75         | \
    B110        | \
    B134        | \
    B150        | \
    B200        | \
    B300        | \
    B600        | \
    B1200       | \
    B1800       | \
    B2400       | \
    B4800       | \
    B9600       | \
    B19200      | \
    B38400      | \
    B57600      | \
    B115200     | \
    B230400       \
)

/*
 * Single bit flag logging
 * */
#define LOG_FLAG_IN_BUF( SBUF, MODE, FLAG ) \
{                                           \
    ASSURE( numberofbits(FLAG) == 1 );      \
    if (MODE & FLAG)                        \
        str_append( SBUF, " " #FLAG);       \
    else                                    \
        str_append( SBUF, " -" #FLAG);      \
}
/*
 * Multiple bit flag logging - negatives suppressed
 * Can also used for single-bit flags where different names apply for
 * enable/disable * */
#define LOG_MFLAG_IN_BUF_NN( SBUF, MODE, ALLBITS, FLAG ) \
{                                           \
    if ((MODE & ALLBITS) == FLAG)           \
        str_append( SBUF, " " #FLAG);       \
}

/*
 * Multiple bit flag logging with shift-left offset for both FLAGs and ALLBITS
 * and prefix string 
 */
#define LOG_FLAGS_IN_BUF( FIELD, SLEFT, SBUF, MODE, ALLBITS, FLAG ) \
{                                                                   \
    if (((MODE << SLEFT) & (ALLBITS << SLEFT) ) == (FLAG << SLEFT)) \
        str_append( SBUF, " " #FIELD "=" #FLAG);                    \
}

#define NBITS_TCFLAG_T ( sizeof(tcflag_t) * 8 )

// Force logging as "Warn" (temporary until deloused)
#ifndef NDEBUG
#  define TCLOG LOGW
#else
#  define TCLOG LOGD
#endif

/*
 * Counts number of bits in a tcflag_t "flag"
 */
static int numberofbits(tcflag_t flag)
{
    int i, cntr = 0;

    for (i = 0; i < NBITS_TCFLAG_T; i++) {
        if (flag & 0x0001)
            cntr++;
        flag >>= 1;
    }

    return cntr;
}

static void str_append(char *dest, const char *apps)
{
    int clength = strlen(dest);

    snprintf(&dest[clength], LOGLINE_MAX, "%s", apps);
}

/* Output given termios struct to log */
void log_termios(const char *leads, struct termios *termios_p)
{

    TCLOG("[%s]: 0x%04X 0x%04X 0x%04X 0x%04X \n", leads, termios_p->c_iflag,
          termios_p->c_oflag, termios_p->c_cflag, termios_p->c_lflag);
    /* I-flags */
    {
        tcflag_t iflag;         /* input modes */
        char sbuf[LOGLINE_MAX] = { 0 };
        iflag = termios_p->c_iflag;

        LOG_FLAG_IN_BUF(sbuf, iflag, BRKINT);
        LOG_FLAG_IN_BUF(sbuf, iflag, ICRNL);
        LOG_FLAG_IN_BUF(sbuf, iflag, IGNBRK);
        LOG_FLAG_IN_BUF(sbuf, iflag, IGNCR);
        LOG_FLAG_IN_BUF(sbuf, iflag, IGNPAR);
        LOG_FLAG_IN_BUF(sbuf, iflag, IMAXBEL);
        LOG_FLAG_IN_BUF(sbuf, iflag, INLCR);
        LOG_FLAG_IN_BUF(sbuf, iflag, INPCK);
        LOG_FLAG_IN_BUF(sbuf, iflag, ISTRIP);
        LOG_FLAG_IN_BUF(sbuf, iflag, IUCLC);
        LOG_FLAG_IN_BUF(sbuf, iflag, IUTF8);
        LOG_FLAG_IN_BUF(sbuf, iflag, IXANY);
        LOG_FLAG_IN_BUF(sbuf, iflag, IXOFF);
        LOG_FLAG_IN_BUF(sbuf, iflag, IXON);
        LOG_FLAG_IN_BUF(sbuf, iflag, PARMRK);
        TCLOG("[%s]: >>> I-flags: %s\n", leads, sbuf);
    }
    /* O-flags */
    {
        tcflag_t oflag;         /* output modes */
        char sbuf[LOGLINE_MAX] = { 0 };
        oflag = termios_p->c_oflag;

        /* DSDLY -- _ BSD_SOURCE */
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (BS0 | BS1), BS0);
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (BS0 | BS1), BS1);

        /* CRDLY -- _ BSD_SOURCE */
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (CR0 | CR1 | CR2 | CR3), CR0);
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (CR0 | CR1 | CR2 | CR3), CR1);
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (CR0 | CR1 | CR2 | CR3), CR2);
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (CR0 | CR1 | CR2 | CR3), CR3);

        /* FFDLY -- _ BSD_SOURCE */
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (FF0 | FF1), FF0);
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (FF0 | FF1), FF1);

        /* NLDLY -- _ BSD_SOURCE */
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (NL0 | NL1), NL0);
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (NL0 | NL1), NL1);

        LOG_FLAG_IN_BUF(sbuf, oflag, OCRNL);
        LOG_FLAG_IN_BUF(sbuf, oflag, OFDEL);
        LOG_FLAG_IN_BUF(sbuf, oflag, OFILL);
        LOG_FLAG_IN_BUF(sbuf, oflag, OLCUC);
        LOG_FLAG_IN_BUF(sbuf, oflag, ONLCR);
        LOG_FLAG_IN_BUF(sbuf, oflag, ONLRET);
        LOG_FLAG_IN_BUF(sbuf, oflag, ONOCR);
        LOG_FLAG_IN_BUF(sbuf, oflag, OPOST);

        /* TABDLY -- _ BSD_SOURCE */
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (TAB0 | TAB1 | TAB2 | TAB3), TAB0);
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (TAB0 | TAB1 | TAB2 | TAB3), TAB1);
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (TAB0 | TAB1 | TAB2 | TAB3), TAB2);
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (TAB0 | TAB1 | TAB2 | TAB3), TAB3);

        /* VTDLY */
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (VT0 | VT1), VT0);
        LOG_MFLAG_IN_BUF_NN(sbuf, oflag, (VT0 | VT1), VT1);

        TCLOG("[%s]: >>> O-flags: %s\n", leads, sbuf);

    }
    /* C-flags */
    {
        tcflag_t cflag;         /* control modes */
        char sbuf[LOGLINE_MAX] = { 0 };
        cflag = termios_p->c_cflag;

#ifndef HAVE_CYGWIN
        LOG_FLAG_IN_BUF(sbuf, cflag, CBAUDEX);
#endif

#ifdef CBAUD /* Just in case.. */
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B0);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B50);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B75);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B110);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B134);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B150);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B200);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B300);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B600);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B1200);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B1800);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B2400);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B4800);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B9600);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B19200);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B38400);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B57600);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B115200);
        LOG_FLAGS_IN_BUF(CBAUD, 0, sbuf, cflag, BAUD_ALL, B230400);
#endif

#ifdef CIBAUD /* Unknown by Cygwin (?) */
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B0);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B50);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B75);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B110);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B134);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B150);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B200);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B300);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B600);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B1200);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B1800);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B2400);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B4800);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B9600);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B19200);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B38400);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B57600);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B115200);
        LOG_FLAGS_IN_BUF(CIBAUD, 16, sbuf, cflag, BAUD_ALL, B230400);
#endif
        LOG_FLAG_IN_BUF(sbuf, cflag, CLOCAL);
#ifdef CMSPAR /* Unknown by Cygwin */
        LOG_FLAG_IN_BUF(sbuf, cflag, CMSPAR);
#endif
        LOG_FLAG_IN_BUF(sbuf, cflag, CREAD);
        LOG_FLAG_IN_BUF(sbuf, cflag, CRTSCTS);
        LOG_FLAG_IN_BUF(sbuf, cflag, CSTOPB);

        LOG_MFLAG_IN_BUF_NN(sbuf, cflag, (CS5 | CS6 | CS7 | CS8), CS5);
        LOG_MFLAG_IN_BUF_NN(sbuf, cflag, (CS5 | CS6 | CS7 | CS8), CS6);
        LOG_MFLAG_IN_BUF_NN(sbuf, cflag, (CS5 | CS6 | CS7 | CS8), CS7);
        LOG_MFLAG_IN_BUF_NN(sbuf, cflag, (CS5 | CS6 | CS7 | CS8), CS8);

        LOG_FLAG_IN_BUF(sbuf, cflag, HUPCL);
#ifdef LOBLK                    /* Not in Linux. */
        LOG_FLAG_IN_BUF(sbuf, cflag, LOBLK);
#endif
        LOG_FLAG_IN_BUF(sbuf, cflag, PARENB);
        LOG_FLAG_IN_BUF(sbuf, cflag, PARODD);
        TCLOG("[%s]: >>> C-flags: %s\n", leads, sbuf);
    }
    /* L-flags */
    {
        tcflag_t lflag;         /* local modes */
        char sbuf[LOGLINE_MAX] = { 0 };
        lflag = termios_p->c_lflag;

#ifdef DEFECHO                  /* Non POSIX. Not in Linux. */
        LOG_FLAG_IN_BUF(sbuf, lflag, DEFECHO);
#endif
        LOG_FLAG_IN_BUF(sbuf, lflag, ECHOCTL);
        LOG_FLAG_IN_BUF(sbuf, lflag, ECHO);
        LOG_FLAG_IN_BUF(sbuf, lflag, ECHOE);
        LOG_FLAG_IN_BUF(sbuf, lflag, ECHOKE);
        LOG_FLAG_IN_BUF(sbuf, lflag, ECHOK);
        LOG_FLAG_IN_BUF(sbuf, lflag, ECHONL);
#ifdef CMSPAR /* Unknown by Cygwin */
        LOG_FLAG_IN_BUF(sbuf, lflag, ECHOPRT);
#endif
        LOG_FLAG_IN_BUF(sbuf, lflag, FLUSHO);
        LOG_FLAG_IN_BUF(sbuf, lflag, ICANON);
        LOG_FLAG_IN_BUF(sbuf, lflag, IEXTEN);
        LOG_FLAG_IN_BUF(sbuf, lflag, ISIG);
        LOG_FLAG_IN_BUF(sbuf, lflag, NOFLSH);
#ifdef CMSPAR /* Unknown by Cygwin */
        LOG_FLAG_IN_BUF(sbuf, lflag, PENDIN);
#endif
        LOG_FLAG_IN_BUF(sbuf, lflag, TOSTOP);
#ifdef CMSPAR /* Unknown by Cygwin */
        LOG_FLAG_IN_BUF(sbuf, lflag, XCASE);
#endif
        TCLOG("[%s]: >>> L-flags: %s\n", leads, sbuf);
    }
}

/* Wrapper as is of system version - but with logging */
static int _tcgetattr(int fd, struct termios *termios_p)
{
    int rc;

    rc = tcgetattr(fd, termios_p);
    log_termios("tcGetattr", termios_p);

    return rc;
}

/* Wrapper as is of system version - but with logging */
static int _tcsetattr(int fd, int optional_actions, struct termios *termios_p)
{
    int rc;

    log_termios("tcSetattr", termios_p);
    rc = tcsetattr(fd, optional_actions, termios_p);

    return rc;
}
/* Set terminal to as doind as little as possible (raw) settings and knwn to
 * be good defaults for BusPirate */
void setserial_raw_bp(int fd)
{
    struct termios c_termios;
    int rc;

    ASSURE(_tcgetattr(fd, &c_termios) == 0);
#ifdef HAVE_CYGWIN
    ASSURE(cfsetspeed(&c_termios, B115200) == 0);
#else
    ASSURE(cfsetspeed(&c_termios, 115200) == 0);
#endif
    rc = cfgetispeed(&c_termios);
    TCLOG("cfgetispeed: %d\n", rc);
    rc = cfgetospeed(&c_termios);
    TCLOG("cfgetospeed: %d\n", rc);

    /* Make it so...*/
    ASSURE(_tcsetattr(fd, TCSANOW, &c_termios) == 0);

    /* Flush pending input.  */
    ASSURE(tcflush(fd, TCIFLUSH) == 0);
}

/* Set terminal settings known to be good defaults for BusPirate */
void setserial_term_bp(int fd)
{
    struct termios c_termios;
    int rc;

    /* Read termios settings. We don't use them, but reading will put current
       values in log */
    ASSURE(_tcgetattr(fd, &c_termios) == 0);

    /* Clear all flags */
    c_termios.c_iflag = 0;
    c_termios.c_oflag = 0;
    c_termios.c_cflag = 0;
    c_termios.c_lflag = 0;


#ifdef NEVER
    /* Rewrite - Good host */
    c_termios.c_iflag = 0x1400;
    c_termios.c_oflag = 0x0000;
    c_termios.c_cflag = 0x14B2;
    c_termios.c_lflag = 0x0000;
#endif

    c_termios.c_cflag |= CS8;
    c_termios.c_cflag |= CREAD;
    c_termios.c_cflag |= HUPCL;

#ifdef NEVER
    //cSmin = 1;
    c_termios.c_cc[VMIN] = 1;   //cSmin;
    c_termios.c_cc[VTIME] = 1;
#endif
  /* Adjust in/out speeds using convenience API */
#ifdef HAVE_CYGWIN
    ASSURE(cfsetspeed(&c_termios, B115200) == 0);
#else
    ASSURE(cfsetspeed(&c_termios, 115200) == 0);
#endif
    rc = cfgetispeed(&c_termios);
    TCLOG("cfgetispeed: %d\n", rc);
    rc = cfgetospeed(&c_termios);
    TCLOG("cfgetospeed: %d\n", rc);

    /* Make it so...*/
    ASSURE(_tcsetattr(fd, TCSANOW, &c_termios) == 0);

    /* Flush pending input.  */
    ASSURE(tcflush(fd, TCIFLUSH) == 0);

    ASSURE(_tcgetattr(fd, &c_termios) == 0);
}
