#include <click/config.h>
#include <time.h>
#include <stdlib.h>
#include <string>
#include "RandomPacketClassifier.hh"
CLICK_DECLS

RandomPacketClassifier::RandomPacketClassifier() : m_probability(0.2f)
{
	srand(time(0));
};

RandomPacketClassifier::~RandomPacketClassifier()
{
};

int
RandomPacketClassifier::configure(Vector<String> &conf, ErrorHandler *errh)
{
    m_probability = std::stof(conf[0], 0);
    return 0;
}

void
RandomPacketClassifier::push(int, Packet *p_in)
{
  float r = (rand() % 1000) / 1000.0;
  if(r < m_probability) {
	  output(0).push(p_in);
  } else {
	  output(1).push(p_in);
  }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(RandomPacketClassifier)
ELEMENT_MT_SAFE(RandomPacketClassifier)