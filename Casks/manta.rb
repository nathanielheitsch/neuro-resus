cask "manta" do
  arch arm: "arm64"

  version "1.6.0"
  sha256 arm: "ee089a9d237cdfc9318834ea3d0c2ee45f2f1de63cd8e759fd362643f95fe238"

  url "https://github.com/nathanielheitsch/neuro-resus/releases/download/manta-v#{version}/manta-v#{version}-macos-#{arch}.tar.gz"
  name "Manta"
  desc "Structural variant and indel caller for germline and somatic analysis"
  homepage "https://github.com/nathanielheitsch/neuro-resus"

  livecheck do
    url :url
    regex(/^manta[._-]v?(\d+(?:\.\d+)+)$/i)
  end

  depends_on arch: :arm64

  binary "manta/bin/configManta.py"
  binary "manta/bin/runMantaWorkflowDemo.py"
end
