import gdown
if __name__ == '__main__':
    output="TNSIM157_3x3s2-f0"
    # download header
    gdown.download(id="1WdOOFfqffoNsHLfTJg6YlkV6W6t_DacT",output=f"{output}.IAEAheader",quiet=False)
    # download actual phsp
    gdown.download(id="1jUsudzDHvVzdHsY9tb9bPgLMoBPnpa0K",output=f"{output}.IAEAphsp",quiet=False)
