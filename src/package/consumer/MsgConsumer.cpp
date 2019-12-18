#include "MsgConsumer.h"


MsgConsumer* MsgConsumer::m_pInst = NULL;
std::mutex MsgConsumer::m_mutex;
