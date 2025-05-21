# https_server

Created HttpClient coding along with this [article](https://osasazamegbe.medium.com/showing-building-an-http-server-from-scratch-in-c-2da7c0db6cb7) by Osamudiamen Azamegbe. Modified it to use HTTPS/TLS as my first addition. Currently WIP

### Original Repo
[See Osamudiamen's Git Repo here](https://github.com/OsasAzamegbe/http-server/tree/main)

### Built with
    *C++
    *Docker
    *WSL2

### Building and Running (Linx/WSL2)

With docker fully setup on your system:

1. Clone the repo and navigate to the directory
2. Install OpenSSL
   ```sh
   apt-get install -y build-essential libssl-dev openssl
   ```
3. Generate a .crt and .key file using OpenSSL 
   ```sh
   openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout server.key -out server.crt
   ```
4. Build the docker image
   ```sh
   docker-compose build
   ```
5. Start the server
   ```sh
   docker-compose up
   ```

To shut down, open a second terminal and enter:
   ```sh
   docker-compose down
   ```  

Any changes made to the code will require the Docker image to be rebuilt.
