import os
import paramiko
import csv
import dotenv
import pickle
from typing import Any, List


propagationModels = ["FriisPropagationLossModel",
                     "ThreeLogDistancePropagationLossModel", "TwoRayGroundPropagationLossModel", "NakagamiPropagationLossModel", "FixedRssLossModel"]


class remote_host:
    def __init__(self, host, port, username, password, ns3dir, timeout) -> None:
        self.ssh = paramiko.SSHClient()
        self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.ssh.connect(host, port, username, password, timeout=timeout)
        self.ssh.get_transport().set_keepalive(60)
        self.sftp = self.ssh.open_sftp()
        self.host = host
        self.port = port
        self.username = username
        self.ns3dir = ns3dir

    def exec(self, command: str) -> str:
        stdin, stdout, stderr = self.ssh.exec_command(command)
        exit_status = stdout.channel.recv_exit_status()
        if exit_status != 0:
            raise Exception(
                f"Command {command} failed with exit status {exit_status}")
        return stdout.read().decode("utf-8")

    def getfile_to(self, remote_path: str, local_path: str) -> None:
        self.sftp.get(remote_path, local_path)
    
    def init_ns3(self) -> None:
        self.exec(f"cd {self.ns3dir} && ./ns3 clean && ./ns3 configure -- --build=optimized && ./ns3 build")

    def __del__(self) -> None:
        self.sftp.close()
        self.ssh.close()

    def __str__(self) -> str:
        return f"{self.host}:{self.port}"


def init_host()-> remote_host:
    dotenv.load_dotenv()
    SSH_USER = os.getenv("SSH_USER")
    SSH_HOST = os.getenv("SSH_HOST")
    SSH_PORT = os.getenv("SSH_PORT")
    SSH_PASSWORD = os.getenv("SSH_PASSWORD")
    NS3_DIR = os.getenv("NS3_DIR")
    return remote_host(host=SSH_HOST, port=SSH_PORT,
                       username=SSH_USER, password=SSH_PASSWORD, ns3dir=NS3_DIR, timeout=60)



class experiment_run:
    def __init__(self, propModel: propagationModels, simulationTime_s: float, distance_m: float, host: remote_host, export_summary: bool, export_rx_pwr: bool, export_rx_dr: bool, remote_csv_path: str, local_csv_path: str, remote_file_cleanup: bool) -> None:
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
        self.host = host
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

    def get_output_files(self) -> None:
        if self.export_summary:
            self.host.getfile_to(self.remote_summary_csv_path,
                                 self.local_summary_csv_path)
        if self.export_rx_pwr:
            self.host.getfile_to(self.remote_rx_pwr_csv_path,
                                 self.local_rx_pwr_csv_path)
        if self.export_rx_dr:
            self.host.getfile_to(self.remote_rx_dr_csv_path,
                                 self.local_rx_dr_csv_path)

    def cleanup_remote_files(self) -> None:
        if not self.remote_file_cleanup:
            return
        if self.export_summary:
            self.host.exec(f"rm {self.remote_summary_csv_path}")
        if self.export_rx_pwr:
            self.host.exec(f"rm {self.remote_rx_pwr_csv_path}")
        if self.export_rx_dr:
            self.host.exec(f"rm {self.remote_rx_dr_csv_path}")
        # deleter folder if empty and exists
        self.host.exec(f"rmdir {self.remote_csv_path}")

    # def __getstate__(self) -> dict:
    #     state = self.__dict__.copy()
    #     del state["host"]
    #     return state

    # def __setattr__(self, *state) -> None:
    #     self.__dict__.update(state)
    #     self.host = init_host()

    def create_output_folders(self) -> None:
        # test if folder exists on remote
        self.host.exec(f"mkdir -p {self.remote_csv_path}")
        # create folder on local
        os.makedirs(self.local_csv_path, exist_ok=True)

    def run(self):
        self.create_output_folders()
        simulation_command = f"{self.host.ns3dir}/ns3 run scratch/wifi-experiment.cc -- --distance={self.distance_m} --simulationTime={self.simulationTime_s} --csv_export_path={self.remote_csv_path} --export_rx_dr={self.str_export_rx_dr} --export_rx_pwr={self.str_export_rx_pwr} --export_summary={self.str_export_summary} --propagationModel={self.propModel}"
        output = self.host.exec(simulation_command)
        print(output)
        self.get_output_files()
        self.cleanup_remote_files()

# TODO: verify


def combine_all_experiment_parameters(propagationModels_list: list[str], distance_m_list: list[float]) -> list[tuple[propagationModels, float]]:
    return [(propModel, distance) for propModel in propagationModels_list for distance in distance_m_list]


def create_experiments(propagationModels_list: List[str], distance_m_list: List[float], simulationTime_s: float, host: remote_host, export_summary: bool, export_rx_pwr: bool, export_rx_dr: bool, remote_csv_path: str, local_csv_path: str) -> List[experiment_run]:
    return [experiment_run(propModel, simulationTime_s, distance, host, export_summary, export_rx_pwr, export_rx_dr, f"{remote_csv_path}_{propModel}_{distance}m", f"{local_csv_path}_{propModel}_{distance}m") for propModel, distance in combine_all_experiment_parameters(propagationModels_list, distance_m_list)]


def hours_to_seconds(hours: float) -> float:
    return hours * 60 * 60


def run_experiments_until_no_dr(simulationTime_s: float, step_size_m: float):
    print("run_experiments_until_no_dr")
    print("init host")
    host = init_host()

    for propagationModel in propagationModels:
        distance = 1.0
        runs = []

        while True:
            path = f"dr_test_{propagationModel}_{distance}m"
            run = experiment_run(host=host, propModel=propagationModel, simulationTime_s=simulationTime_s, distance_m=distance,
                                 remote_csv_path=f"{host.ns3dir}/{path}", local_csv_path=f"{path}", export_rx_dr=True, export_summary=True, export_rx_pwr=False, remote_file_cleanup=True)
            runs.append(run)
            print(f"running experiment for {distance} meters with {propagationModel} propagation")
            try:
                run.run()
            except Exception as e:
                print(e)
                break
            print("done")
            # open summary.csv and check if there is any dr (calculating mean is offloaded to ns3)
            with open(run.local_summary_csv_path, "r") as f:
                # create csv parser
                reader = csv.DictReader(f)
                # check the datarate column for zero values
                dr = [float(row["ReceivedDataRate"]) for row in reader]
                if all([d == 0.00 or d==0.01 for d in dr]):
                    break
            distance += step_size_m
        # pickle.dump(runs, open(f"runs_{propagationModel}.pickle", "wb"))

def test_datarate_over_time(start_time_s: int, end_time_s:int, init_ns3:bool=True ):
    print("test_datarate_over_time")
    print("init host")
    host = init_host()
    if init_ns3:
        print("init ns3")
        host.init_ns3()
    runs = []
    for time_s in range(start_time_s,end_time_s):
        time_s_f = float(time_s)
        path = f"time_increment_test_{time_s}s"
        run = experiment_run(host=host, propModel=propagationModels[0], simulationTime_s=time_s_f, distance_m=10,
                         remote_csv_path=f"{host.ns3dir}/{path}", local_csv_path=f"{path}", export_rx_dr=True, export_summary=True, export_rx_pwr=True, remote_file_cleanup=True)
        runs.append(run)
        print(f"running experiment for {time_s} seconds")
        run.run()
        print("done")
    return runs


if __name__ == "__main__":
    import dotenv
    import os
    dotenv.load_dotenv()
    SSH_USERNAME = os.getenv("SSH_USERNAME")
    SSH_HOST = os.getenv("SSH_HOST")
    SSH_PORT = os.getenv("SSH_PORT")
    SSH_PASSWORD = os.getenv("SSH_PASSWORD")
    NS3_DIR = os.getenv("NS3_DIR")
    propModel = propagationModels[0]

    host = remote_host(host=SSH_HOST, port=SSH_PORT,
                       username=SSH_USERNAME, password=SSH_PASSWORD, ns3dir=NS3_DIR, timeout=60)
    run = experiment_run(host=host, propModel=propModel, simulationTime_s=10, distance_m=10,
                         remote_csv_path=f"{host.ns3dir}/sample", local_csv_path="sample", export_rx_dr=True, export_summary=True, export_rx_pwr=True, remote_file_cleanup=False)
    print(run)
    run.run()


