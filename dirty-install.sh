export HL2SDKCS2=$PWD/sdk/; export METAMOD112=$PWD/metamod-source/; export PATH="${HOME}/.local/bin:$PATH"
bash docker-entrypoint.sh && cp -r dockerbuild/package/addons/* ~/.steam/steam/steamapps/common/Counter-Strike\ Global\ Offensive/game/csgo/addons/
