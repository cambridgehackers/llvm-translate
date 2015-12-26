class l_class_OC_Fifo {
  class l_class_OC_PipeIn in;
  class l_class_OC_PipeOut out;
public:
  void deq(void);
  bool deq__RDY(void);
  void enq(unsigned int enq_v);
  bool enq__RDY(void);
  unsigned int first(void);
  bool first__RDY(void);
};

class l_class_OC_Fifo1 {
  class l_class_OC_PipeIn in;
  class l_class_OC_PipeOut out;
  unsigned int element;
  bool full;
public:
  void deq(void);
  bool deq__RDY(void);
  void enq(unsigned int enq_v);
  bool enq__RDY(void);
  unsigned int first(void);
  bool first__RDY(void);
};

