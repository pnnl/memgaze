# Instructions to run Darknet to Train Classifiers

1. Run the makefile simply by 'make' [Unless you're want to change the compiler options, default optimization flags, etc]
2. Run the script train_networks.sh. The idea here is to pass the task that you're about to do (Train a classifier), on the dataset and your classifier network as arguments to the executable generated from the last step:

   ./darknet  classifier train path-to-your-data.cfg path-to-your-network.cfg  -> This is exactly what the script does on a bunch of networks

# How to get the *.cfg files

1. The network architecture file (path-to-your-network.cfg) should already by there on the darknet/cfg folder. 
2. The data cfg file is essentially a file that points to your dataset in darknet/data folder. It has to have a specific format, so just copy the following to a file called cifar-small.cfg and put it in the darknet/cfg folder (& edit the path names):

******************
classes=10
train  = /home/bodhisatwa/Research_Repos/benchmarks/darknet/data/cifar-small/train.list
valid  = /home/bodhisatwa/Research_Repos/benchmarks/darknet/data/cifar-small/test.list
labels = /home/bodhisatwa/Research_Repos/benchmarks/darknet/data/cifar-small/labels.txt
backup = /home/bodhisatwa/Research_Repos/benchmarks/darknet/backup/
top=2
*********************

# How to make sure your training doesn't take forever (~ weeks) to run

1. For every network that you're training on, go to its cfg [for alexnet, it's darket/cfg/alexnet.cfg] and change its "max_batches" variables. In our case, we had 'max_batches=100', which meant that for 20 training images, the entire network will train for 20 * 5 = 100 epochs
