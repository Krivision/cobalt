// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_APPLICATIONS_ISOLATED_WEB_APPS_INSTALL_ISOLATED_WEB_APP_COMMAND_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_ISOLATED_WEB_APPS_INSTALL_ISOLATED_WEB_APP_COMMAND_H_

#include <memory>
#include <ostream>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/strings/string_piece_forward.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/web_applications/commands/web_app_command.h"
#include "chrome/browser/web_applications/isolated_web_apps/isolated_web_app_location.h"
#include "chrome/browser/web_applications/isolated_web_apps/isolated_web_app_response_reader_factory.h"
#include "chrome/browser/web_applications/isolated_web_apps/isolated_web_app_url_info.h"
#include "chrome/browser/web_applications/os_integration/os_integration_manager.h"
#include "chrome/browser/web_applications/web_app_id.h"
#include "chrome/browser/web_applications/web_app_install_info.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/webapps/browser/install_result_code.h"
#include "components/webapps/browser/installable/installable_logging.h"
#include "third_party/blink/public/mojom/manifest/manifest.mojom-forward.h"

class GURL;
class Profile;
class PrefService;

namespace content {
class WebContents;
}  // namespace content

namespace web_app {

class AppLock;
class AppLockDescription;
class LockDescription;
class WebAppDataRetriever;
class WebAppUrlLoader;

enum class WebAppUrlLoaderResult;

struct InstallIsolatedWebAppCommandSuccess {};
struct InstallIsolatedWebAppCommandError {
  std::string message;

  friend std::ostream& operator<<(
      std::ostream& os,
      const InstallIsolatedWebAppCommandError& error) {
    return os << "InstallIsolatedWebAppCommandError { message = \""
              << error.message << "\" }.";
  }
};

// Isolated Web App requires:
//  * no cross-origin navigation
//  * content should never be loaded in normal tab
//
// |content::IsolatedWebAppThrottle| enforces that. The requirements prevent
// re-using web contents.
class InstallIsolatedWebAppCommand : public WebAppCommandTemplate<AppLock> {
 public:
  static std::unique_ptr<IsolatedWebAppResponseReaderFactory>
  CreateDefaultResponseReaderFactory(const PrefService& prefs);

  // |url_info| holds the origin information of the app. It is randomly
  // generated for dev-proxy and the public key of signed bundle. It is
  // guarantee to be valid.
  //
  // |location| holds information about the mode(dev-mod-proxy/signed-bundle)
  // and the source.
  //
  // |callback| must be not null.
  //
  // The `id` in the application's manifest must equal "/".
  //
  // `response_reader_factory` should be created via
  // `CreateDefaultResponseReaderFactory` and is used to create the
  // `IsolatedWebAppResponseReader` for the Web Bundle.
  explicit InstallIsolatedWebAppCommand(
      const IsolatedWebAppUrlInfo& url_info,
      const IsolatedWebAppLocation& location,
      std::unique_ptr<content::WebContents> web_contents,
      std::unique_ptr<WebAppUrlLoader> url_loader,
      std::unique_ptr<ScopedKeepAlive> keep_alive,
      std::unique_ptr<ScopedProfileKeepAlive> profile_keep_alive,
      base::OnceCallback<
          void(base::expected<InstallIsolatedWebAppCommandSuccess,
                              InstallIsolatedWebAppCommandError>)> callback,
      std::unique_ptr<IsolatedWebAppResponseReaderFactory>
          response_reader_factory);

  InstallIsolatedWebAppCommand(const InstallIsolatedWebAppCommand&) = delete;
  InstallIsolatedWebAppCommand& operator=(const InstallIsolatedWebAppCommand&) =
      delete;

  InstallIsolatedWebAppCommand(InstallIsolatedWebAppCommand&&) = delete;
  InstallIsolatedWebAppCommand& operator=(InstallIsolatedWebAppCommand&&) =
      delete;

  ~InstallIsolatedWebAppCommand() override;

  // WebAppCommandTemplate<AppLock>:
  const LockDescription& lock_description() const override;
  base::Value ToDebugValue() const override;
  void StartWithLock(std::unique_ptr<AppLock> lock) override;
  void OnSyncSourceRemoved() override;
  void OnShutdown() override;

  void SetDataRetrieverForTesting(
      std::unique_ptr<WebAppDataRetriever> data_retriever);

 private:
  void ReportFailure(base::StringPiece message);
  void ReportSuccess();

  Profile& profile();
  const PrefService& prefs();

  void DownloadIcons(WebAppInstallInfo install_info);
  void OnGetIcons(WebAppInstallInfo install_info,
                  IconsDownloadedResult result,
                  std::map<GURL, std::vector<SkBitmap>> icons_map,
                  std::map<GURL, int /*http_status_code*/> icons_http_results);

  void CheckTrustAndSignaturesOfBundle(const base::FilePath& path);
  void OnTrustAndSignaturesChecked(
      base::expected<void, IsolatedWebAppResponseReaderFactory::Error> result);

  void CreateStoragePartition();

  void LoadUrl();
  void OnLoadUrl(WebAppUrlLoaderResult result);

  void CheckInstallabilityAndRetrieveManifest();
  void OnCheckInstallabilityAndRetrieveManifest(
      blink::mojom::ManifestPtr opt_manifest,
      const GURL& manifest_url,
      bool valid_manifest_for_web_app,
      webapps::InstallableStatusCode error_code);
  base::expected<WebAppInstallInfo, std::string> CreateInstallInfoFromManifest(
      const blink::mojom::Manifest& manifest,
      const GURL& manifest_url);
  void FinalizeInstall(const WebAppInstallInfo& info);
  void OnFinalizeInstall(const AppId& unused_app_id,
                         webapps::InstallResultCode install_result_code,
                         OsHooksErrors unused_os_hooks_errors);

  SEQUENCE_CHECKER(sequence_checker_);

  std::unique_ptr<AppLockDescription> lock_description_;
  std::unique_ptr<AppLock> lock_;

  IsolatedWebAppUrlInfo url_info_;
  IsolatedWebAppLocation location_;

  std::unique_ptr<IsolatedWebAppResponseReaderFactory> response_reader_factory_;

  std::unique_ptr<content::WebContents> web_contents_;

  std::unique_ptr<WebAppUrlLoader> url_loader_;

  std::unique_ptr<ScopedKeepAlive> keep_alive_;
  std::unique_ptr<ScopedProfileKeepAlive> profile_keep_alive_;

  std::unique_ptr<WebAppDataRetriever> data_retriever_;

  base::OnceCallback<void(base::expected<InstallIsolatedWebAppCommandSuccess,
                                         InstallIsolatedWebAppCommandError>)>
      callback_;

  base::WeakPtrFactory<InstallIsolatedWebAppCommand> weak_factory_{this};
};

}  // namespace web_app

#endif  // CHROME_BROWSER_WEB_APPLICATIONS_ISOLATED_WEB_APPS_INSTALL_ISOLATED_WEB_APP_COMMAND_H_
