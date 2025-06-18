/** @type {import('next').NextConfig} */

// const nextConfig = {
//   experimental: {

//   },
// };

const path = require("path");

module.exports = {
  webpack: (config) => {
    config.resolve.alias["@"] = path.resolve(__dirname, "app");
    return config;
  },
};

//module.exports = nextConfig;
