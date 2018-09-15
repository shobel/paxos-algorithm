all: Proposer Acceptor Learner

Proposer: Proposer.cpp
	g++ -o Proposer Proposer.cpp -lnsl -lresolv

Acceptor: Acceptor.cpp
	g++ -o Acceptor Acceptor.cpp -lnsl -lresolv

Learner: Learner.cpp
	g++ -o Learner Learner.cpp -lnsl -lresolv
