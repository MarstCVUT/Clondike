#ifndef _PROXYFS_IOCTL_H
#define _PROXYFS_IOCTL_H


#define IOCTL_READ   0x40000000
#define IOCTL_WRITE  0x80000000
#define IOCTL_RW     ( IOCTL_READ | IOCTL_WRITE )

#define IOCTL_WILL_WRITE(arg)	( (arg) & IOCTL_WRITE )
#define IOCTL_WILL_READ(arg)	( (arg) & IOCTL_READ )
#define IOCTL_ARG_SIZE(arg)	( (arg) & 0x0FFFffff )

extern const unsigned long const ioctl_table[];
extern const unsigned long ioctl_table_nmemb;

static inline unsigned long ioctl_get_info(int cmd){
	int low = 0;
	int high = ioctl_table_nmemb - 1;
	int mid;

	for(mid = high/2; low != high; mid = low + ((high - low) / 2)){
		if( cmd < ioctl_table[mid*2] )
			high = mid;
		else if( cmd > ioctl_table[mid*2] )
			low = mid + 1;
		else
			return ioctl_table[mid*2+1];
	}
	if( cmd == ioctl_table[low*2] )
		return ioctl_table[low*2+1];
	else
		return 0;
}

#ifdef PROXYFS_IOCTL_PRIVATE
#define IOCTL_INT    0x30000004
#define IOCTL_VOID   0x30000000
#define IOCTL_CHAR   0x30000003


#endif /* PROXYFS_IOCTL_PRIVATE */

#endif
