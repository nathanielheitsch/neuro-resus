cask "wham" do
  arch arm: "arm64"

  version "1.0.0"
  sha256 "fc7f5098962b26e419dcd3159dba93d1e20296cbe5c29dc75d0e620f019e28ab"

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

  # ponytail: quarantine strip until Developer ID + notarization are available;
  # the Apple Development signature stays valid, Gatekeeper just skips the
  # "Apple could not verify" check for locally-trusted installs. Switch to
  # notarized binaries and remove this when the paid program cert exists.
  postflight do
    system_command "xattr", args: ["-dr", "com.apple.quarantine", staged_path]
  end

  binary "wham"
end
