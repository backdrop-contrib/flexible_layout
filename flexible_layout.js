(function ($) {
Backdrop.behaviors.tester = {
  attach: function() {
    $('#edit-default-converted-result').click(function() {
     this.select();
    });
  }
}

})(jQuery);
