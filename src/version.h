#ifndef VERSION_H
#define VERSION_H

struct ttsVersion
{
    int major;		///< significant changes
    int minor;		///< incremental changes
    int revision;	///< bug fixes
};

extern ttsVersion g_ttsVersion;

#endif // VERSION_H
