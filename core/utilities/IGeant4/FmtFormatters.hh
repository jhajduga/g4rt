// FmtFormatters.hh
#pragma once

#include <fmt/format.h>
#include <CLHEP/Vector/ThreeVector.h>
#include <CLHEP/Vector/Rotation.h>

template <>
struct fmt::formatter<CLHEP::HepRotation> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const CLHEP::HepRotation& r, FormatContext& ctx) const{
        return fmt::format_to(ctx.out(),
            "[\n  [{:.3f}, {:.3f}, {:.3f}],\n  [{:.3f}, {:.3f}, {:.3f}],\n  [{:.3f}, {:.3f}, {:.3f}]\n]",
            r.xx(), r.xy(), r.xz(),
            r.yx(), r.yy(), r.yz(),
            r.zx(), r.zy(), r.zz());
    }
};

template <>
struct fmt::formatter<CLHEP::Hep3Vector> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const CLHEP::Hep3Vector& v, FormatContext& ctx) const{
        return fmt::format_to(ctx.out(), "({:.3f}, {:.3f}, {:.3f})", v.x(), v.y(), v.z());
    }
};
