export default {
  testEnvironment: 'jsdom',
  testMatch: ['**/tests/**/*.test.js'],
  transform: {},
  moduleFileExtensions: ['js'],
  collectCoverageFrom: ['public/js/**/*.js', 'public/*.js'],
  injectGlobals: true,
};
