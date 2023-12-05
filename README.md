# KVS_ns3_lab
A student project to practise using the ns3 network simulator

## getting started

### 1. install ns3
This project assumes a working ns-3 simulator is present. Version 3.39 was used for this assignment,
so preferably install this version if you want to reproduce it. Please refer to the ns3 manual for 
installation details.

### 2. clone this repo
You probably have a folder full of your git repos. Set the GIT_FOLDER env var accordingly.

```
export GIT_FOLDER=$HOME/git
mkdir $GIT_FOLDER
cd $GIT_FOLDER
git clone https://github.com/MainManu/KVS_ns3_lab
cd KVS_ns3_lab
export REPO_FOLDER=$PWD
```

### 3. link the repo scratch folder into the ns3 scratch folder

Assuming you followed the ns3 tutorial, you should already have set up an env variable that points to 
the ns3 base folder called NS3DIR. If not, do so now.

#### 3.1 backup your scratch folder (just in case)

```
mv $NS3DIR/scratch $NS3DIR/scratch_backup
```

#### 3.2 link the folder

```
ln -s $REPO_FOLDER/scratch $NS3DIR
``
