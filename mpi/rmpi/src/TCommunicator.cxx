#include<Mpi/TCommunicator.h>
#include <Mpi/TMpiMessage.h>
#include<iostream>
#include<TSystem.h>
#include<TROOT.h>
using namespace ROOT::Mpi;

ROOT::Mpi::TCommunicator *gComm = new ROOT::Mpi::TCommunicator(MPI_COMM_WORLD);
ROOT::Mpi::TCommunicator COMM_WORLD(MPI_COMM_WORLD);

//______________________________________________________________________________
template<> void TCommunicator::Serialize<TMpiMessage>(Char_t **buffer, Int_t &size, const TMpiMessage *vars, Int_t count, const TCommunicator *comm, Int_t dest, Int_t source, Int_t tag, Int_t root)
{
   std::vector<TMpiMessageInfo> msgis(count);
   for (auto i = 0; i < count; i++) {
      auto mbuffer = vars[i].Buffer();
      auto msize   = vars[i].BufferSize();
      if (mbuffer == NULL) {
         comm->Error(__FUNCTION__, "Error serializing object type %s \n", ROOT_MPI_TYPE_NAME(TMpiMessage));
         comm->Abort(ERR_BUFFER);
      }
      TMpiMessageInfo msgi(mbuffer, msize);
      msgi.SetSource(comm->GetRank());
      msgi.SetDestination(dest);
      msgi.SetSource(source);
      msgi.SetRoot(root);
      msgi.SetTag(tag);
      msgi.SetDataTypeName(ROOT_MPI_TYPE_NAME(TMpiMessage));
      msgis[i] = msgi;
   }
   TMpiMessage msg;
   msg.WriteObject(msgis);
   auto ibuffer = msg.Buffer();
   size = msg.BufferSize();
   *buffer = new Char_t[size];
   if (ibuffer == NULL) {
      comm->Error(__FUNCTION__, "Error serializing object type %s \n", ROOT_MPI_TYPE_NAME(msgis));
      comm->Abort(ERR_BUFFER);
   }
   memcpy(*buffer, ibuffer, size);

}


//______________________________________________________________________________
template<> void TCommunicator::Unserialize<TMpiMessage>(Char_t *ibuffer, Int_t isize, TMpiMessage *vars, Int_t count, const TCommunicator *comm, Int_t dest, Int_t source, Int_t tag, Int_t root)
{
   TMpiMessage msg(ibuffer, isize);
   auto cl = gROOT->GetClass(typeid(std::vector<TMpiMessageInfo>));
   auto msgis = (std::vector<TMpiMessageInfo> *)msg.ReadObjectAny(cl);
   if (msgis == NULL) {
      comm->Error(__FUNCTION__, "Error unserializing object type %s \n", cl->GetName());
      comm->Abort(ERR_BUFFER);
   }

   if (msgis->data()->GetDataTypeName() != ROOT_MPI_TYPE_NAME(TMpiMessage)) {
      comm->Error(__FUNCTION__, "Error unserializing objects type %s where objects are %s \n", ROOT_MPI_TYPE_NAME(TMpiMessage), msgis->data()->GetDataTypeName().Data());
      comm->Abort(ERR_TYPE);
   }

   ROOT_MPI_ASSERT(msgis->data()->GetDestination() == dest, comm)
   ROOT_MPI_ASSERT(msgis->data()->GetSource() == source, comm)
   ROOT_MPI_ASSERT(msgis->data()->GetRoot() == root, comm)
   ROOT_MPI_ASSERT(msgis->data()->GetTag() == tag, comm)

   for (auto i = 0; i < count; i++) {
      //passing information from TMpiMessageInfo to TMpiMessage
      auto size = msgis->data()[i].GetBufferSize();
      Char_t *buffer = new Char_t[size];//this memory dies when the unserialized object dies
      memcpy(buffer, msgis->data()[i].GetBuffer(), size);
      vars[i].SetBuffer(buffer, size, kFALSE);
      vars[i].SetReadMode();
      vars[i].Reset();
   }
}



//______________________________________________________________________________
/**
 * Copy constructor for communicator
 * \param comm other TCommunicator object
 */
TCommunicator::TCommunicator(const TCommunicator &comm): TObject(comm)
{
   fComm = comm.fComm;
   fMainProcess = comm.fMainProcess;
}

//______________________________________________________________________________
TCommunicator::TCommunicator(const MPI_Comm &comm): fComm(comm), fMainProcess(0) {}

//______________________________________________________________________________
TCommunicator::~TCommunicator()
{
}

//______________________________________________________________________________
/**
 *         Method for synchronization between MPI processes in a communicator
 */
void  TCommunicator::Barrier() const
{
   MPI_Barrier(fComm);
}

//______________________________________________________________________________
/**
 *         Method for synchronization between MPI nonblocking processes in a communicator
 *         \param req request object
 */
void  TCommunicator::IBarrier(TRequest &req) const
{
   MPI_Ibarrier(fComm, &req.fRequest);
   if (req.fRequest == MPI_REQUEST_NULL) req.fCallback();
}

//______________________________________________________________________________
/**
 *            Nonblocking test for a message. Operations  allow checking of incoming messages without actual receipt of them.
 *              \param source Source rank or ROOT::Mpi::ANY_SOURCE (integer).
 *              \param tag Tag value or ROOT::Mpi::ANY_TAG (integer).
 *              \param status TStatus object with extra information.
 *              \return boolean true if the probe if ok
 */
Bool_t TCommunicator::IProbe(Int_t source, Int_t tag, TStatus &status) const
{
   Int_t flag;
   MPI_Status stat;
   MPI_Iprobe(source, tag, fComm, &flag, &stat);
   status = stat;
   return (Bool_t)flag;
}

//______________________________________________________________________________
/**
 *            Nonblocking test for a message. Operations  allow checking of incoming messages without actual receipt of them.
 *              \param source Source rank or ROOT::Mpi::ANY_SOURCE (integer).
 *              \param tag Tag value or ROOT::Mpi::ANY_TAG (integer).
 *              \return boolean true if the probe if ok
 */
Bool_t TCommunicator::IProbe(Int_t source, Int_t tag) const
{
   Int_t flag;
   MPI_Status status;
   MPI_Iprobe(source, tag, fComm, &flag, &status);
   return (Bool_t)flag;
}

//______________________________________________________________________________
/**
 *            Test for a message. Operations  allow checking of incoming messages without actual receipt of them.
 *              \param source Source rank or ROOT::Mpi::ANY_SOURCE (integer).
 *              \param tag Tag value or ROOT::Mpi::ANY_TAG (integer).
 *              \param status TStatus object with extra information.
 *              \return boolean true if the probe if ok
 */
void TCommunicator::Probe(Int_t source, Int_t tag, TStatus &status) const
{
   MPI_Status stat;
   MPI_Probe(source, tag, fComm, &stat);
   status = stat;
}

//______________________________________________________________________________
/**
 *            Test for a message. Operations  allow checking of incoming messages without actual receipt of them.
 *              \param source Source rank or ROOT::Mpi::ANY_SOURCE (integer).
 *              \param tag Tag value or ROOT::Mpi::ANY_TAG (integer).
 *              \return boolean true if the probe if ok
 */
void TCommunicator::Probe(Int_t source, Int_t tag) const
{
   MPI_Status stat;
   MPI_Probe(source, tag, fComm, &stat);
}

