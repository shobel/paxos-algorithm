# paxos-algorithm

I have created a system of processes that communicate with each other over TCP, and through an algorithm called Paxos, they arrive at a consensus that gets distributed throughout the system. More specifically, proposals consisting of a phase, number, and value are created by the proposer components. They send their proposals to the acceptors who reply with the previous proposal they have received (or some value like "no previous" if no previous proposal has been accepted). After the proposers hear back from the majority of acceptors, they find the highest numbered proposal they've heard back from the acceptors about and re-send out their proposal in the "accept" phase. The acceptors accept the proposal if they have not replied to a "prepare" proposal with a higher number. Upon accepting a proposal, the acceptor forwards the message to the learners.

## Code files and functionality:
There are 3 code files: Proposer.cpp, Acceptor.cpp, and Learner.cpp. Proposer.cpp forks off some child processes. Each child process acts as a proposer and carries out the duties of a proposer described above in an infinite loop and are terminated when their parent process terminates (via CTRL+C). The general design of the Acceptors and Learners functionality is exactly like the Proposer, but they carry out their specific protocols.

## To Compile:
make all

## To Run:
There are 2 ways to run these programs. You can run them with no arguments, or you can run them with the number of desired Proposers Acceptors and Receivers as the 1st 2nd and 3rd argument to each program, eg ./Proposer 5 5 5 will tell the program that you desire 5 of each component. Not all components need to know about the number of all 3 components, but for simplicity, they all expect no arguments or the same arguments. If run with no arguments, default values of 2 3 1 are be given, as used in the guidelines.

./executable [optional arguments...]

./Learner [num_proposers] [num_acceptors] [num_learners]
./Acceptor [num_proposers] [num_acceptors] [num_learners]
./Proposer [num_proposers] [num_acceptors] [num_learners]

The order is not all that important. Ideally the acceptor and learner are listening when the proposer starts, but they will timeout and send new proposals if they don't hear back from the acceptors. The learner should be running before the acceptor, though, because the whole process will likely not appear successful if the acceptor accepts a value and sends to a learner that hasn't started running yet.

## Output Format:
All components will preface each output by declaring who they are and following it with their message, ie "Component#: message". The "Component" part is replaced by just the first letter of the actual component, for example Proposers use "P" and Acceptors use "A" etc. The "#" is replaced with the number of the child process, eg the first proposer process will preface with P0:. The messages describe the proposal details, who sent it, and who should receive it. Sometimes they contain generic actions like timeouts or listening messages.

## Idiosyncracies:
Proposers will attempt to send until they have all been accepted and thus will sometimes continue sending even after the majority of acceptors have notified the learners of an accepted proposal. The learners will (purposefully) stop receiving messages and exit after they have heard a proposal from all acceptors. The proposers have knowledge of when their proposals are accepted rather than ignored and will stop sending proposals after they are accepted by the majority.
 
