class l_class_OC_Module {
public:
};

class l_class_OC_Fifo {
public:
};

class l_class_OC_Fifo1 {
public:
  unsigned int element;
  bool full;
  bool enq__RDY(void);
  void enq(unsigned int v);
  bool deq__RDY(void);
  void deq(void);
  bool first__RDY(void);
  unsigned int first(void);
};

class l_class_OC_Echo {
public:
  class l_class_OC_Fifo1 fifo;
  class l_class_OC_EchoIndication *ind;
  unsigned int pipetemp;
  void rule_respond(void);
  bool echoReq__RDY(void);
  void echoReq(unsigned int v);
  bool rule_respond__RDY(void);
  void run();
};

