struct header
{
  unsigned int sequencenum : 32;
  unsigned int awknum : 32;
  unsigned int connid : 16;
  unsigned int notused : 13;
  unsigned int A : 1;
  unsigned int S : 1;
  unsigned int F : 1;
};