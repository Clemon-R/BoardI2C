#ifndef CLIENTSTATE_H_
#define CLIENTSTATE_H_

typedef enum	ClientState_e {
    NONE = 0,
    DEINITIATIED,
    DEINITIATING,
    DISCONNECTING,
    DISCONNECTED,
    INITIATING,
    INITIATIED,
    CONNECTING,
    CONNECTED
}				ClientState_t;

#endif