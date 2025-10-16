# Container image info
The canonical build image is currently `bennellickeng/fedora-zephyr:f42_0.17.1-rc1-2` It is publicly available and so can be used by developers in scratch repos. This is hosted on docker_hub and sometimes takes a while to pull, presumably due to the ephemeral nature of the runners, a bit like a chache miss.

Images hosted on GHCR are supposedly co-located with the runners and so significantly faster to pull. Hence, to speed up the build we can copy the image to a GHCR repo.

## Steps to copy image from public repo to private repo
At the moment, it is not wort the effort to automate this, as it is a one-off task. However, if the image is updated frequently, then it would be worth automating.
1. Create a Personal Access Token (classic):
    1. Click your profile photo in the upper-right corner of GitHub
    2. Navigate to: (Your user) Settings → Developer settings → Personal access tokens
    3. Select: Tokens (classic) → Generate new token → Generate new token (classic)
    4. Set a short lifetime (only needs to be short-lived)
    5. Select the `write:packages` scope

2. Copy token to env variable and authenticate:
   ```bash
   export CR_PAT=<TOKEN>
   echo $CR_PAT | podman login ghcr.io -u tbennellick --password-stdin
   ```

3. Tag and push the image:
   ```bash
   podman tag bennellickeng/fedora-zephyr:f42_0.17.1-rc1-2 ghcr.io/stowood/fedora-zephyr:f42_0.17.1-rc1-2
   podman push ghcr.io/stowood/fedora-zephyr:f42_0.17.1-rc1-2
   ```