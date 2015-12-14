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

class l_class_OC_EchoIndication {
public:
  unsigned long long unused_data_to_flag_indication_echo;
  void echo(unsigned int v);
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
  void respond_rule(void);
  bool respond_rule__RDY(void);
  void run();
};

class l_class_OC_EchoTest {
public:
  class l_class_OC_Echo *echo;
  unsigned int x;
};

