cask "wham" do
  arch arm: "arm64"

  version "1.0.0"
  sha256 "6ee0ba3b8764268c8dbf3b08e795b89eeca709654ade24d05909b6cadfcb4437"

  url "https://github.com/nathanielheitsch/neuro-resus/releases/download/wham-v#{version}/wham-v#{version}-macos-#{arch}.tar.gz"
  name "WHAM"
  desc "Structural variant caller that uses linked-read and split-read evidence"
  homepage "https://github.com/nathanielheitsch/neuro-resus"

  livecheck do
    url :url
    regex(/^wham[._-]v?(\d+(?:\.\d+)+)$/i)
  end

  depends_on :macos
  depends_on arch: :arm64


  binary "wham"
end
