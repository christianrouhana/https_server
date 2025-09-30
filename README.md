# https_server

Created a simple HTTP server coding along with this [article](https://osasazamegbe.medium.com/showing-building-an-http-server-from-scratch-in-c-2da7c0db6cb7) by Osamudiamen Azamegbe. Modified it to use HTTPS/TLS and now am converting it to an Old School Runescape Tool. Currently WIP

5/23/2025
- Added a file to handle a client that automatically sends a request to the server. Runs simultaneously with the server
5/24/2025
- Reworked the server into an HTTPS Old School RuneScape hiscore proxy that fetches a single player over HTTPS.

### Next Steps
- Harden the HTTPS API (better routing, structured logging, graceful shutdown).
- Add caching or persistence to track changes over time.
- Build a lightweight frontend for browsing individual skill breakdowns.

### Original Repo
[See Osamudiamen's Git Repo here](https://github.com/OsasAzamegbe/http-server/tree/main)

### Built with
    *C++
    *Docker
    *WSL2
    *OpenSSL

### Building and Running (Linux/WSL2)

With docker fully setup on your system:

1. Clone the repo and navigate to the directory
2. Install OpenSSL
   ```sh
   apt-get install -y libssl-dev openssl
   ```
3. Generate a .crt and .key file using OpenSSL 
   ```sh
   openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout server.key -out server.crt
   ```
4. Build the docker image
   ```sh
   docker-compose build
   ```
5. Provide a certificate bundle so the tracker can trust `secure.runescape.com`. Either export a PEM bundle (for example with `curl` or `openssl`), or point the code at your system bundle. At runtime the server reads the `OSRS_CA_BUNDLE` environment variable; if unset it falls back to `cacert.pem` in the project root.
6. Start the server
   ```sh
   docker-compose up
   ```

Navigate to https://localhost:443/ to see results. The root path returns a small help message. The main endpoint accepts a RuneScape display name:

```
https://localhost:443/player?name=Zezima
```

### OSRS Hiscore API

- `GET /` – simple help payload.
- `GET /player?name=Display%20Name` – fetches the hiscore entry for the supplied player and returns their skill ranks, levels, and experience as JSON. If the name is missing or the player cannot be found, the endpoint returns an error JSON payload.

Responses follow this shape:

```json
{
  "name": "Zezima",
  "success": true,
  "skills": {
    "Overall": {"rank": 12345, "level": 2277, "experience": 1234567890}
  }
}
```

To shut it down down, open a second terminal and enter:
   ```sh
   docker-compose down
   ```  

Any changes made to the code will require the Docker image to be rebuilt.
