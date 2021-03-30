#include "buildjson.hpp"
#include "general_systemd.hpp"
#include "skip_action.hpp"

#include <nlohmann/json.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{
using ::testing::IsEmpty;

using json = nlohmann::json;

TEST(FirmwareJsonTest, InvalidHandlerType)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "unsupported",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
         }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, InvalidPreparationType)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "superfun",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
         }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, InvalidVerificationType)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "funtimes",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
         }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, InvalidUpdateType)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "systemd"
                }
            }
         }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, MissingHandler)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
        }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, MissingActions)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            }
        }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, MissingActionPreparation)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
        }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, MissingActionVerification)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
        }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, MissingActionUpdate)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                }
            }
        }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, TwoConfigsOneInvalidReturnsValid)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
         },
         {
            "blob" : "/flash/image2",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
        }]
    )"_json;

    auto h = buildHandlerFromJson(j2);
    EXPECT_EQ(h[0].blobId, "/flash/image2");
    EXPECT_EQ(h.size(), 1);
}

/*
 * TODO: It may be worth individually using builders per type, and testing
 * those.
 *
 * TODO: Only allow unique handler blob paths (tested at a higher level).
 */

TEST(FirmwareJsonTest, VerifyBlobNameMatches)
{
    /* A perfect configuration except the blob name doesn't start with "/flash/"
     */
    auto j2 = R"(
        [{
            "blob" : "bmc-image-flash",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
         }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, VerifyMinimumBlobNameLength)
{
    /* A perfect configuration except the blob name is effectively zero length.
     */
    auto j2 = R"(
        [{
            "blob" : "/flash/",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
         }]
    )"_json;

    EXPECT_THAT(buildHandlerFromJson(j2), IsEmpty());
}

TEST(FirmwareJsonTest, VerifySystemdWithReboot)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "reboot"
                }
            }
         }]
    )"_json;

    auto h = buildHandlerFromJson(j2);
    EXPECT_EQ(h[0].blobId, "/flash/image");
    EXPECT_FALSE(h[0].handler == nullptr);
    EXPECT_FALSE(h[0].actions == nullptr);
    EXPECT_FALSE(h[0].actions->preparation == nullptr);
    EXPECT_FALSE(h[0].actions->verification == nullptr);
    EXPECT_FALSE(h[0].actions->update == nullptr);
}

TEST(FirmwareJsonTest, VerifyMultipleHandlersReturned)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-update.target"
                }
            }
        },
        {
            "blob" : "/flash/bios",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-update.target"
                }
            }
        }]
    )"_json;

    auto h = buildHandlerFromJson(j2);
    EXPECT_EQ(h.size(), 2);
    EXPECT_EQ(h[0].blobId, "/flash/image");
    EXPECT_EQ(h[1].blobId, "/flash/bios");
}

TEST(FirmwareJsonTest, VerifyValidSingleNonReboot)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify"
                },
                "update" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-update.target"
                }
            }
         }]
    )"_json;

    auto h = buildHandlerFromJson(j2);
    EXPECT_EQ(h[0].blobId, "/flash/image");
    EXPECT_FALSE(h[0].handler == nullptr);
    EXPECT_FALSE(h[0].actions == nullptr);
    EXPECT_FALSE(h[0].actions->preparation == nullptr);
    EXPECT_FALSE(h[0].actions->verification == nullptr);
    auto verifier = reinterpret_cast<SystemdWithStatusFile*>(
        h[0].actions->verification.get());
    EXPECT_THAT(verifier->getMode(), "replace");
    EXPECT_FALSE(h[0].actions->update == nullptr);
    auto updater = reinterpret_cast<SystemdNoFile*>(h[0].actions->update.get());
    EXPECT_THAT(updater->getMode(), "replace");
}

TEST(FirmwareJsonTest, VerifyValidWithModes)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify",
                    "mode" : "replace-nope"
                },
                "update" : {
                    "type" : "systemd",
                    "mode" : "replace-fake",
                    "unit" : "phosphor-ipmi-flash-bmc-update.target"
                }
            }
         }]
    )"_json;

    auto h = buildHandlerFromJson(j2);
    EXPECT_EQ(h[0].blobId, "/flash/image");
    EXPECT_FALSE(h[0].handler == nullptr);
    EXPECT_FALSE(h[0].actions == nullptr);
    EXPECT_FALSE(h[0].actions->preparation == nullptr);
    EXPECT_FALSE(h[0].actions->verification == nullptr);
    auto verifier = reinterpret_cast<SystemdWithStatusFile*>(
        h[0].actions->verification.get());
    EXPECT_THAT(verifier->getMode(), "replace-nope");
    EXPECT_FALSE(h[0].actions->update == nullptr);
    auto updater = reinterpret_cast<SystemdNoFile*>(h[0].actions->update.get());
    EXPECT_THAT(updater->getMode(), "replace-fake");
}

TEST(FirmwareJsonTest, VerifyValidUpdateWithFilePath)
{
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "systemd",
                    "unit" : "phosphor-ipmi-flash-bmc-prepare.target"
                },
                "verification" : {
                    "type" : "fileSystemdVerify",
                    "unit" : "phosphor-ipmi-flash-bmc-verify.target",
                    "path" : "/tmp/bmc.verify",
                    "mode" : "replace-nope"
                },
                "update" : {
                    "type" : "fileSystemdUpdate",
                    "mode" : "replace-fake",
                    "unit" : "phosphor-ipmi-flash-bmc-update.target",
                    "path" : "/tmp/update.verify"
                }
            }
         }]
    )"_json;

    auto h = buildHandlerFromJson(j2);
    EXPECT_EQ(h[0].blobId, "/flash/image");
    EXPECT_FALSE(h[0].handler == nullptr);
    EXPECT_FALSE(h[0].actions == nullptr);
    EXPECT_FALSE(h[0].actions->preparation == nullptr);
    EXPECT_FALSE(h[0].actions->verification == nullptr);
    auto verifier = reinterpret_cast<SystemdWithStatusFile*>(
        h[0].actions->verification.get());
    EXPECT_THAT(verifier->getMode(), "replace-nope");
    EXPECT_FALSE(h[0].actions->update == nullptr);
    auto updater =
        reinterpret_cast<SystemdWithStatusFile*>(h[0].actions->update.get());
    EXPECT_THAT(updater->getMode(), "replace-fake");
}

TEST(FirmwareJsonTest, VerifySkipFields)
{
    // In this configuration, nothing happens because all actions are set to
    // skip.
    auto j2 = R"(
        [{
            "blob" : "/flash/image",
            "handler" : {
                "type" : "file",
                "path" : "/run/initramfs/bmc-image"
            },
            "actions" : {
                "preparation" : {
                    "type" : "skip"
                },
                "verification" : {
                    "type" : "skip"
                },
                "update" : {
                    "type" : "skip"
                }
            }
         }]
    )"_json;

    auto h = buildHandlerFromJson(j2);
    EXPECT_EQ(h[0].blobId, "/flash/image");
    EXPECT_FALSE(h[0].handler == nullptr);
    EXPECT_FALSE(h[0].actions == nullptr);
    EXPECT_FALSE(h[0].actions->preparation == nullptr);
    EXPECT_FALSE(h[0].actions->verification == nullptr);
    EXPECT_FALSE(h[0].actions->update == nullptr);
}

} // namespace
} // namespace ipmi_flash
