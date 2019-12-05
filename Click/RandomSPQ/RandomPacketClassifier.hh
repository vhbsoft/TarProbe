#ifndef CLICK_RANDOMPACKETCLASSIFIER_HH
#define CLICK_RANDOMPACKETCLASSIFIER_HH
#include <click/element.hh>

//
// This element takes an IP packet and randomly classifies the packet as a high priority packet
//
CLICK_DECLS
class RandomPacketClassifier : public Element
{
public:
  RandomPacketClassifier();
  ~RandomPacketClassifier();

  const char *class_name() const {return "RandomPacketClassifier";}
  const char *port_count() const {return "1/2";}
  const char *processing() const {return PUSH;}

  int configure(Vector<String> &conf, ErrorHandler *) CLICK_COLD;
  void push(int, Packet *);


private:
  float m_probability;
};
CLICK_ENDDECLS
#endif