import dotenv
import os
import pandas as pd
from wifi_experiment.utlis import *
from .utlis import *

# load environment variables
dotenv.load_dotenv()
SSH_USERNAME = os.getenv("SSH_USERNAME")
SSH_HOST = os.getenv("SSH_HOST")
SSH_PORT = os.getenv("SSH_PORT")
SSH_PASSWORD = os.getenv("SSH_PASSWORD")
NS3DIR = os.getenv("NS3DIR")

# connect to remote host
host = remote_host(host=SSH_HOST, port=SSH_PORT, username=SSH_USERNAME, password=SSH_PASSWORD, ns3dir=NS3DIR, timeout=hours_to_seconds(
    2))  # this might usually be a bad idea, but the simulation runs a long time, so the timeout is set to a ridiculous value


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
