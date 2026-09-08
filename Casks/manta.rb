cask "manta" do
  arch arm: "arm64"

  version "1.6.0"
  sha256 "08705e8a833b481ba8b63c32c8ceb11f46dd45cc026f742eb2951eb07756efa9"

  url "https://github.com/nathanielheitsch/neuro-resus/releases/download/manta-v#{version}/manta-v#{version}-macos-#{arch}.tar.gz"
  name "Manta"
  desc "Structural variant and indel caller for germline and somatic analysis"
  homepage "https://github.com/nathanielheitsch/neuro-resus"

  livecheck do
    url :url
    regex(/^manta[._-]v?(\d+(?:\.\d+)+)$/i)
  end

  depends_on :macos
  depends_on arch: :arm64

  # ponytail: quarantine strip until Developer ID + notarization are available;
  # switch to notarized binaries and remove this when the paid cert exists.
  postflight do
    system_command "xattr", args: ["-dr", "com.apple.quarantine", staged_path]
  end

  binary "manta/bin/configManta.py"
  binary "manta/bin/runMantaWorkflowDemo.py"
end
