//
// Created by cc 23-6-23.
//

#ifndef DNS_DEBUG_H
#define DNS_DEBUG_H

#define debug 1
#define dbg_print
#define dbg(format, ...) if(debug) if (debugLevel > 0) { printf("%s %s File %s Line %d : ", __DATE__, __TIME__, __FILE__, __LINE__);printf(format, ##__VA_ARGS__);printf("\n");}
#define ddbg(format, ...) if(debug) if (debugLevel > 1) { printf("%s %s File %s Line %d : ", __DATE__, __TIME__, __FILE__, __LINE__);printf(format, ##__VA_ARGS__);printf("\n");};

extern short debugLevel;

void setDebugLevel(short level);

#endif //DNS_DEBUG_H
