# Official SDK download: automated alternative to local compilers

- Correct SDK for RTL8721CSM / GW006: https://github.com/Ameba-AIoT/ameba-rtos-d
- The newer repository `ameba-rtos` is for other, newer variants (e.g. RTL8721Dx); don't mix the API sets.
- In the current ChatGPT container, outbound DNS for github.com is blocked, so the complete vendor repository has **not** been downloaded and no device `.bin` has been produced.
- The GitHub Actions workflow `.github/workflows/sdk-fetch-audit.yml` can clone the official SDK on GitHub-hosted infrastructure after this project is added to a GitHub repository and the workflow is run.
- The workflow reads source code, lists IR headers and APIs and runs host unit tests. It **does not** compile Realtek application images, flash the GW006 or claim device compatibility.
- The workflow generates `sdk_commit.txt` and `sdk_audit.json` as audit artifacts. This pins the source revision for subsequent development.
- Next milestone after audit: integrate the real HAL, Wi-Fi and image build targets from the cloned repository, then attempt a native KM0/KM4 build in the same hosted environment.
- Before any physical flash: full device backup, boot map, recovery path, image integrity, and explicit user permission.
