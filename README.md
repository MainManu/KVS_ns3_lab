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
```

Now you can work within ns3 as usual, and track the scratch folder using this repo.

### 4. run the simulation

```
./$NS3DIR/ns3 run scratch/wifi-experiment.cc -- [args]
```
Please refer to the project report for a list of available arguments.


## Project report

The project report is written in latex. Its source files are located in the report/ folder. 
To build the document run build.sh. Make sure to install texlive first. Depending on your package 
manager you might need to install additional subpackages.

## python scripts

The python scripts are located in the DataAnalysis folder. To innstall all dependencies run

```
pip3 install -U pipenv
pipenv install
```

To run the scripts, first activate the virtual environment

```
pipenv shell
```

### setting up a .env file
In order to run the scripts, you need to set up a .env file in the DataAnalysis folder.
This should contain the following variables:

```
SSH_HOST="hostname"
SSH_USER="username"
SSH_PORT="port"
SSH_PASSWORD="pw"
NS3_DIR="path to ns3 folder on remote host"
```

### running the notebooks


The actual data analysis is located in initial_experiment.ipynb for the steady state determination and dr_pwr_over_distance_experiment.ipynb for the distance to power ratio experiment. 
The python wrapper for the ns3 simulation is located in the wifi-experiment module located in the respectively named folder.
You might need to run the experiments first using the testing_wifi_experiment.ipynb notebook. This will generate the data files needed for the analysis.
An example of how the output should look like is located in the sample_data folder.