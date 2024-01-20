import dotenv
import os
import pandas as pd
from .utils import *

# load environment variables
dotenv.load_dotenv()
SSH_USERNAME = os.getenv("SSH_USERNAME")
SSH_HOST = os.getenv("SSH_HOST")
SSH_PORT = os.getenv("SSH_PORT")
SSH_PASSWORD = os.getenv("SSH_PASSWORD")
NS3_DIR = os.getenv("NS3_DIR")

# connect to remote host
# host = remote_host(host=SSH_HOST, port=SSH_PORT, username=SSH_USERNAME, password=SSH_PASSWORD, ns3dir=NS3DIR, timeout=hours_to_seconds(
# 2))  # this might usually be a bad idea, but the simulation runs a long time, so the timeout is set to a ridiculous value


def restore_runs():
    simulationTime_s = 10
    runs = []
    # restore friis runs
    propagationModel = "FriisPropagationLossModel"
    for distance_m_friis in range(1, 231, 1):
        distance = float(distance_m_friis)
        runs.append(experiment_run(host=host, propModel=propagationModel, simulationTime_s=simulationTime_s, distance_m=distance,
                                   remote_csv_path=f"{host.ns3dir}/output/{propagationModel}_{distance}_m", local_csv_path=f"output/{propagationModel}_{distance}_m", export_rx_dr=True, export_summary=True, export_rx_pwr=True, remote_file_cleanup=True))

    # restore threelogdistance runs
    propagationModel = "ThreeLogDistancePropagationLossModel"
    for distance_m_threelog in range(1, 249, 1):
        distance = float(distance_m_threelog)
        runs.append(experiment_run(host=host, propModel=propagationModel, simulationTime_s=simulationTime_s, distance_m=distance,
                                   remote_csv_path=f"{host.ns3dir}/output/{propagationModel}_{distance}_m", local_csv_path=f"output/{propagationModel}_{distance}_m", export_rx_dr=True, export_summary=True, export_rx_pwr=True, remote_file_cleanup=True))

    # restore TwoRayGroundPropagationLossModel runs
    propagationModel = "TwoRayGroundPropagationLossModel"
    for distance_m_twoRay in range(1, 232, 1):
        distance = float(distance_m_twoRay)
        runs.append(experiment_run(host=host, propModel=propagationModel, simulationTime_s=simulationTime_s, distance_m=distance,
                                   remote_csv_path=f"{host.ns3dir}/output/{propagationModel}_{distance}_m", local_csv_path=f"output/{propagationModel}_{distance}_m", export_rx_dr=True, export_summary=True, export_rx_pwr=True, remote_file_cleanup=True))

    # restore Nagakami runs
    propagationModel = "NakagamiPropagationLossModel"
    for distance_m_nagakami in range(1, 2040, 1):
        distance = float(distance_m_nagakami)
        runs.append(experiment_run(host=host, propModel=propagationModel, simulationTime_s=simulationTime_s, distance_m=distance,
                                   remote_csv_path=f"{host.ns3dir}/output/{propagationModel}_{distance}_m", local_csv_path=f"output/{propagationModel}_{distance}_m", export_rx_dr=True, export_summary=True, export_rx_pwr=True, remote_file_cleanup=True))

    return runs


def merge_dr_to_dataframe(runs: List[experiment_run]) -> pd.DataFrame:
    df = pd.DataFrame()


def cm_to_inches(cm: float) -> float:
    return cm / 2.54


def restore_test_datarate_over_time(start_time_s: int, end_time_s: int, init_ns3: bool = True, folder: str = "output"):
    runs = []
    dotenv.load_dotenv()
    SSH_USERNAME = os.getenv("SSH_USERNAME")
    SSH_HOST = os.getenv("SSH_HOST")
    SSH_PORT = os.getenv("SSH_PORT")
    SSH_PASSWORD = os.getenv("SSH_PASSWORD")
    NS3_DIR = os.getenv("NS3_DIR")

    for time_s in range(start_time_s, end_time_s):
        time_s_f = float(time_s)
        path = f"{folder}/time_increment_test_{time_s}s"
        run = restored_experiment_run(propModel=propagationModels[0], simulationTime_s=time_s_f, distance_m=10,
                                      remote_csv_path=f"{NS3_DIR}/{path}", local_csv_path=f"{path}", export_rx_dr=True, export_summary=True, export_rx_pwr=True, remote_file_cleanup=True)
        runs.append(run)
        print(f"running experiment for {time_s} seconds")
        # run.run()
        print("done")
    return runs


def restore_test_datarate_over_distance(folder: str = "output"):
    runs = []
    dotenv.load_dotenv()
    NS3_DIR = os.getenv("NS3_DIR")

    for propagationModel in ["FriisPropagationLossModel",
                             "ThreeLogDistancePropagationLossModel", "TwoRayGroundPropagationLossModel", "NakagamiPropagationLossModel", "FixedRssLossModel"]:
        distance_m = 1
        while True:
            distance_m_f = float(distance_m)
            path = f"{folder}/{propagationModel}_{distance_m_f}_m"
            run = restored_experiment_run(propModel=propagationModel, simulationTime_s=10, distance_m=distance_m_f,
                                          remote_csv_path=f"{NS3_DIR}/{path}", local_csv_path=f"{path}", export_rx_dr=True, export_summary=True, export_rx_pwr=True, remote_file_cleanup=True)
            # check if summary.csv exists
            if os.path.exists(run.local_csv_path):
                distance_m += 1
                runs.append(run)
            else:
                break
    return runs


class restored_experiment_run:
    def __init__(self, propModel: propagationModels, simulationTime_s: float, distance_m: float,  export_summary: bool, export_rx_pwr: bool, export_rx_dr: bool, remote_csv_path: str, local_csv_path: str, remote_file_cleanup: bool) -> None:
        if propModel not in propagationModels:
            raise ValueError("propModel must be in propagationModels")
        self.propModel = propModel
        if distance_m < 1:
            raise ValueError("distance_m must be bigger than 1")
        if simulationTime_s < 0:
            raise ValueError("simulationTime_s must be bigger than 0")
        self.remote_csv_path = remote_csv_path
        self.local_csv_path = local_csv_path
        self.simulationTime_s = simulationTime_s
        self.distance_m = distance_m
        self.export_summary = export_summary
        self.export_rx_pwr = export_rx_pwr
        self.export_rx_dr = export_rx_dr
        self.str_export_summary = self.bool_to_cpp_str(export_summary)
        self.str_export_rx_pwr = self.bool_to_cpp_str(export_rx_pwr)
        self.str_export_rx_dr = self.bool_to_cpp_str(export_rx_dr)
        self.remote_file_cleanup = remote_file_cleanup
        if self.export_summary:
            self.remote_summary_csv_path = f"{self.remote_csv_path}/summary.csv"
            self.local_summary_csv_path = f"{self.local_csv_path}/summary.csv"
        if self.export_rx_pwr:
            self.remote_rx_pwr_csv_path = f"{self.remote_csv_path}/pwr.csv"
            self.local_rx_pwr_csv_path = f"{self.local_csv_path}/pwr.csv"
        if self.export_rx_dr:
            self.remote_rx_dr_csv_path = f"{self.remote_csv_path}/dr.csv"
            self.local_rx_dr_csv_path = f"{self.local_csv_path}/dr.csv"

    def bool_to_cpp_str(self, b: bool) -> str:
        return "true" if b else "false"
