cask "manta" do
  arch arm: "arm64"

  version "1.6.0"
  sha256 "b0fc4efa2ae0dbbe58fc3be67b901c913ebdbe179efe21303c279a241a923a0c"

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


  binary "manta/bin/configManta.py"
  binary "manta/bin/runMantaWorkflowDemo.py"
end
