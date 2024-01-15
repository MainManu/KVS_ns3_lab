import paramiko

propagationModels = ["FriisPropagationLossModel", "FixedRssLossModel",
                     "ThreeLogDistancePropagationLossModel", "TwoRayGroundPropagationLossModel", "NakagamiPropagationLossModel"]


class remote_host:
    def __init__(self, host, port, username, password, ns3dir) -> None:
        self.ssh = paramiko.SSHClient()
        self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.ssh.connect(host, port, username, password)
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

    def getfile(self, remote_path: str, local_path: str) -> None:
        self.sftp.get(remote_path, local_path)

    def __del__(self) -> None:
        self.sftp.close()
        self.ssh.close()

    def __str__(self) -> str:
        return f"{self.host}:{self.port}"


class experiment_run:
    def __init__(self, propModel: propagationModels, simulationTime_s: float, distance_m: float, host: remote_host, remote_csv_path: str, local_csv_path: str) -> None:
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

    def run(self):
        output = self.host.exec(
            f"{self.host.ns3dir}/ns3 run scratch/wifi-experiment.cc -- --distance={self.distance_m} --simulationTime={self.simulationTime_s} --csv_export_path={self.remote_csv_path} --propagationModel={self.propModel}")
        print(output)
        self.host.getfile(self.remote_csv_path, self.local_csv_path)
