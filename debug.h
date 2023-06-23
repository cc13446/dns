//
// Created by cc 23-6-23.
//

#ifndef DNS_DEBUG_H
#define DNS_DEBUG_H

#define debug 1
#define dbg_print
#define dbg(format, ...) if(debug) if (debugLevel > 0) { printf("%s : ", __DATE__); printf(format, ##__VA_ARGS__);}
#define ddbg(format, ...) if(debug) if (debugLevel > 1) { printf("%s : ", __DATE__); printf(format, ##__VA_ARGS__);}

extern short debugLevel;

void setDebugLevel(short level);

#endif //DNS_DEBUG_H
