#include <linux/types.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/time.h>
#include <asm/socket.h>
#include <asm/termios.h>

#define PROXYFS_IOCTL_PRIVATE
#include "proxyfs_ioctl.h"

const unsigned long const ioctl_table[] = {
	// <include/asm-i386/socket.h>
	FIOSETOWN,			sizeof( const int )			| IOCTL_WRITE,
	SIOCSPGRP,			sizeof( const int )			| IOCTL_WRITE,
	FIOGETOWN,			sizeof( int )				| IOCTL_READ,
	SIOCGPGRP,			sizeof( int )				| IOCTL_READ,
	SIOCATMARK,			sizeof( int )				| IOCTL_READ,
	SIOCGSTAMP,			sizeof( struct timeval )		| IOCTL_READ,

	// <include/asm-i386/termios.h>
	TCGETS,				sizeof( struct termios )		| IOCTL_READ,
	TCSETS,				sizeof( const struct termios )		| IOCTL_WRITE,
	TCSETSW,			sizeof( const struct termios )		| IOCTL_WRITE,
	TCSETSF,			sizeof( const struct termios )		| IOCTL_WRITE,
	TCGETA,				sizeof( struct termio )			| IOCTL_READ,
	TCSETA,				sizeof( const struct termio )		| IOCTL_WRITE,
	TCSETAW,			sizeof( const struct termio )		| IOCTL_WRITE,
	TCSETAF,			sizeof( const struct termio )		| IOCTL_WRITE,
	TCSBRK,				IOCTL_INT,
	TCXONC,				IOCTL_INT,
	TCFLSH,				IOCTL_INT,
	TIOCEXCL,			IOCTL_VOID,
	TIOCNXCL,			IOCTL_VOID,
	TIOCSCTTY,			IOCTL_INT,
	TIOCGPGRP,			sizeof( pid_t )				| IOCTL_READ,
	TIOCSPGRP,			sizeof( const pid_t )			| IOCTL_WRITE,
	TIOCOUTQ,			sizeof( int )				| IOCTL_READ,
	TIOCSTI,			sizeof( const char * )			| IOCTL_WRITE, // FIXME
	TIOCGWINSZ,			sizeof( struct winsize )		| IOCTL_READ,
	TIOCSWINSZ,			sizeof( const struct winsize )		| IOCTL_WRITE,
	TIOCMGET,			sizeof( int )				| IOCTL_READ,
	TIOCMBIS,			sizeof( const int )			| IOCTL_WRITE,
	TIOCMBIC,			sizeof( const int )			| IOCTL_WRITE,
	TIOCMSET,			sizeof( const int )			| IOCTL_WRITE,
	TIOCGSOFTCAR,			sizeof( int )				| IOCTL_READ,
	TIOCSSOFTCAR,			sizeof( const int ) 			| IOCTL_WRITE,
	FIONREAD,			sizeof( int )				| IOCTL_READ,
	TIOCINQ,			sizeof( int )				| IOCTL_READ,
	TIOCLINUX,			sizeof( const char )			| IOCTL_WRITE, // MORE
	TIOCCONS,			IOCTL_VOID,
	TIOCGSERIAL,			sizeof( struct serial_struct )		| IOCTL_READ,
	TIOCSSERIAL,			sizeof( const struct serial_struct )	| IOCTL_WRITE,
	TIOCPKT,			sizeof( const int )			| IOCTL_WRITE,
	FIONBIO,			sizeof( const int )			| IOCTL_WRITE,
	TIOCNOTTY,			IOCTL_VOID,
	TIOCSETD,			sizeof( const int )			| IOCTL_WRITE,
	TIOCGETD,			sizeof( int )				| IOCTL_READ,
	TCSBRKP,			IOCTL_INT,
	//TIOCTTYGSTRUCT,			sizeof( struct tty_struct )		| IOCTL_READ,
	FIONCLEX,			IOCTL_VOID,
	FIOCLEX,			IOCTL_VOID,
	FIOASYNC,			sizeof( const int )			| IOCTL_WRITE,
	TIOCSERCONFIG,			IOCTL_VOID,
	TIOCSERGWILD,			sizeof( int )				| IOCTL_READ,
	TIOCSERSWILD,			sizeof( const int )			| IOCTL_WRITE,
	TIOCGLCKTRMIOS,			sizeof( struct termios )		| IOCTL_READ,
	TIOCSLCKTRMIOS,			sizeof( const struct termios )		| IOCTL_WRITE,
	//TIOCSERGSTRUCT,			sizeof( struct async_struct )		| IOCTL_READ,
	TIOCSERGETLSR,			sizeof( int )				| IOCTL_READ,
	TIOCSERGETMULTI,		sizeof( struct serial_multiport_struct )| IOCTL_READ,
	TIOCSERSETMULTI,		sizeof( const struct serial_multiport_struct  )		| IOCTL_WRITE,
};

const unsigned long ioctl_table_nmemb = sizeof(ioctl_table)/(2*sizeof(unsigned long));
