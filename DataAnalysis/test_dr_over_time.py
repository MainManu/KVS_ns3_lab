from wifi_experiment import *
import pickle

runs = test_datarate_over_time(init_ns3=False, start_time_s=70, end_time_s=5*60)
pickle.dump(runs, open("runs.pickle", "wb"))