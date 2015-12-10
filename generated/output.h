class l_class_OC_Module {
public:
};

class l_class_OC_Fifo {
public:
  void deq(void);
  bool deq__RDY(void);
  void enq(unsigned int v);
  bool enq__RDY(void);
  unsigned int first(void);
  bool first__RDY(void);
};

class l_class_OC_Fifo1 {
public:
  unsigned int element;
  bool full;
  void deq(void);
  bool deq__RDY(void);
  void enq(unsigned int v);
  bool enq__RDY(void);
  unsigned int first(void);
  bool first__RDY(void);
};

class l_class_OC_Echo {
public:
  class l_class_OC_Fifo1 fifo;
  class l_class_OC_EchoIndication *ind;
  unsigned int pipetemp;
  void echoReq(unsigned int v);
  bool echoReq__RDY(void);
  void rule_respond(void);
  bool rule_respond__RDY(void);
  void run();
};

