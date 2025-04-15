# Getting started

* Install [Node.js](https://nodejs.org/download/)
* Install grunt: `npm install -g grunt-cli`
* Install the Node.js dependencies via npm: `npm install`
* Install Ruby; See [this](https://jekyllrb.com/docs/installation/windows/) guide
* Run `gem install bundle` and then `bundle install`
* Run `grunt build` to build the static site, `grunt` to build and watch for changes (http://localhost:8000/)
* To test the site, run `npm test`

* workaround if 'grunt build' fails:
* uncomment 'clean' and 'jekyll' in Gruntfile.js in the grunt.registerTask('build'
* run `bundle exec jekyll build` and then `grunt build`
* to test the website, `npm i -g serve`, then run `bundle exec jekyll serve` and
* in another console run `serve dist`

## TODO:

1. Move posts into `_posts`
2. Add jekyll-feed
