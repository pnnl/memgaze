

declare -a benchs1=( alexnet resnet18 resnet34 resnet50 resnet101 resnet152 )
for i in "${benchs1[@]}"
do
  echo "Training $i with on our custom CIFAR small dataset"       
  ./darknet classifier train cfg/cifar-small.data cfg/$i.cfg
done




