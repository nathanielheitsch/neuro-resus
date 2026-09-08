cask "wham" do
  arch arm: "arm64"

  version "1.0.0"
  sha256 arm: "8e063e84429f6a553ede05b3d95aad4a406141f2fef4880783d062920f312f84"

  url "https://github.com/nathanielheitsch/neuro-resus/releases/download/wham-v#{version}/wham-v#{version}-macos-#{arch}.tar.gz"
  name "WHAM"
  desc "Structural variant caller that uses linked-read and split-read evidence"
  homepage "https://github.com/nathanielheitsch/neuro-resus"

  livecheck do
    url :url
    regex(/^wham[._-]v?(\d+(?:\.\d+)+)$/i)
  end

  depends_on arch: :arm64

  binary "wham"
end
